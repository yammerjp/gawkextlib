/*
 * lmdb.c - Builtin functions to implement lmdb.
 */

/*
 * Copyright (C) 2016 the Free Software Foundation, Inc.
 * 
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software
 * Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston, MA  02110-1301, USA
 */

#include "common.h"
#include <lmdb.h>
#include <assert.h>

#define API_ERROR (MDB_LAST_ERRCODE-1)

static const struct {
   const char *name;
   int val;
} mdbdef[] = {
#define init_lmdb(A, B, C) { #B, C },
#include "lmdb_header.h"
#undef init_lmdb
};

static struct {
   awk_scalar_t cookie;
   awk_value_t value;
} mdb_errno;

static struct {
  struct namespace {
    strhash *ht;
    size_t n;
    char name[7];
  } env, txn, dbi, cursor;
} mdb = {
  .env = { .name = "env" },
  .txn = { .name = "txn" },
  .dbi = { .name = "dbi" },
  .cursor = { .name = "cursor" },
};

/* for use with mdb_cursor_get */
static awk_value_t ksub, dsub;

/* N.B. we assume that the caller is passing an AWK_NUMBER */
static inline awk_bool_t
is_int(awk_value_t *x)
{
  long y;
  double z;

  y = x->num_value;
  z = y;
  return (z == x->num_value);
}

static inline awk_bool_t
is_uint(awk_value_t *x)
{
  return (x->num_value >= 0) && is_int(x);
}

static awk_value_t *
do_mdb_strerror(int nargs, awk_value_t *result)
{
  awk_value_t err;
  const char *s;

  if (do_lint && nargs > 1)
    lintwarn(ext_id, _("%s: called with too many arguments"), __func__+3);
  
  if (!get_argument(0, AWK_NUMBER, &err) || !is_int(&err)) {
    set_ERRNO(_("mdb_strerror: argument must be an integer error number"));
    RET_NULSTR;
  }
  if (err.num_value == API_ERROR)
    s = _("API_ERROR: internal error in gawk lmdb API");
  else
    s = mdb_strerror(err.num_value);
  return make_string_malloc(s, strlen(s), result);
}

static strhash_entry *
get_handle(struct namespace *ns, awk_value_t *name, const char *funcname)
{
  strhash_entry *hte;
  char handle[sizeof(ns->name)+24];

  snprintf(handle, sizeof(handle), "%s%zu", ns->name, ns->n++);
  name->str_value.len = strlen(handle);
  hte = strhash_get(ns->ht, handle, name->str_value.len, 1);
  if (hte->data)
    fatal(ext_id, _("%s: hash %s corruption detected: handle %s is not unique"),
	  funcname, ns->name, handle);
  name->str_value.str = hte->s;
  return hte;
}

static void
release_handle(struct namespace *ns, awk_value_t *name, const char *funcname)
{
  if (strhash_delete(ns->ht, name->str_value.str, name->str_value.len,
		     NULL, NULL) < 0)
    fatal(ext_id, _("%s: unable to release %s handle `%s'"),
	  funcname, ns->name, name->str_value.str);
}

static void *
lookup_handle(struct namespace *ns, size_t argnum, awk_value_t *key,
	      awk_bool_t empty_ok, const char *funcname)
{
  strhash_entry *hte;
  awk_value_t keybuf;
  
  if (!key)
    /* some callers need the name so they can release the handle */
    key = &keybuf;
  if (!get_argument(argnum, AWK_STRING, key)) {
    char emsg[256];
    snprintf(emsg, sizeof(emsg),
	     _("%s: argument #%zu must be a string identifying the %s"),
	     funcname, argnum+1, ns->name);
    set_ERRNO(emsg);
    return NULL;
  }
  if (!key->str_value.len) {
    if (!empty_ok) {
      char emsg[256];
      snprintf(emsg, sizeof(emsg),
	       _("%s: argument #%zu empty string invalid as a %s handle"),
	       funcname, argnum+1, ns->name);
      set_ERRNO(emsg);
    }
    return NULL;
  }
  if (!(hte = strhash_get(ns->ht, key->str_value.str, key->str_value.len, 0))) {
    char emsg[256];
    snprintf(emsg, sizeof(emsg),
	     _("%s: argument #%zu `%s' does not map to a known %s handle"),
	     funcname, argnum+1, key->str_value.str, ns->name);
    set_ERRNO(emsg);
    return NULL;
  }
  if (!hte->data)
    fatal(ext_id,
	  _("%s: corruption detected: %s handle `%s' maps to a NULL pointer"),
	  funcname, ns->name, key->str_value.str);
  return hte->data;
}

static inline void
update_mdb_errno(awk_value_t *x)
{
  if (!sym_update_scalar(mdb_errno.cookie, x))
    fatal(ext_id, _("unable to update MDB_ERRNO value"));
}

static inline int
set_mdb_errno(int rc)
{
  mdb_errno.value.num_value = rc;
  update_mdb_errno(&mdb_errno.value);
  return rc;
}

#define SET_AND_RET(rc) {		\
  result->val_type = AWK_NUMBER;	\
  result->num_value = rc;		\
  update_mdb_errno(result);		\
  return result;			\
}

static awk_value_t *
do_mdb_version(int nargs, awk_value_t *result)
{
  const char *s;
  int rc = MDB_SUCCESS;
  int val[3];

  if (do_lint && nargs > 1)
    lintwarn(ext_id, _("%s: called with too many arguments"), __func__+3);
  s = mdb_version(&val[0], &val[1], &val[2]);
  if (nargs > 0) {
    awk_value_t arr;
    if (!get_argument(0, AWK_ARRAY, &arr)) {
      set_ERRNO(_("mdb_version: optional 1st argument must be an array"));
      rc = API_ERROR;
    }
    else {
      static const char *name[] = { "major", "minor", "patch" };
      size_t i;
      clear_array(arr.array_cookie);
      for (i = 0; i < 3; i++) {
	awk_value_t x, y;
        if (!set_array_element(arr.array_cookie,
			       make_string_malloc(name[i], strlen(name[i]), &x),
			       make_number(val[i], &y))) {
	  set_ERRNO(_("mdb_version: set_array_element failed"));
	  rc = API_ERROR;
	}
      }
    }
  }
  set_mdb_errno(rc);
  return make_string_malloc(s, strlen(s), result);
}

static awk_value_t *
do_mdb_env_create(int nargs, awk_value_t *result)
{
  awk_value_t name;
  MDB_env *env;

  if (do_lint && nargs > 0)
    lintwarn(ext_id, _("%s: called with too many arguments"), __func__+3);
  if (set_mdb_errno(mdb_env_create(&env)) != MDB_SUCCESS) {
    set_ERRNO(_("mdb_env_create failed"));
    RET_NULSTR;
  }
  get_handle(&mdb.env, &name, __func__+3)->data = env;
  return make_string_malloc(name.str_value.str, name.str_value.len, result);
}

static awk_value_t *
do_mdb_env_close(int nargs, awk_value_t *result)
{
  awk_value_t name;
  MDB_env *env;
  int rc;

  if (do_lint && nargs > 1)
    lintwarn(ext_id, _("%s: called with too many arguments"), __func__+3);
  if (!(env = lookup_handle(&mdb.env, 0, &name, awk_false, __func__+3)))
    rc = API_ERROR;
  else {
    mdb_env_close(env);
    release_handle(&mdb.env, &name, __func__+3);
    rc = MDB_SUCCESS;
  }
  SET_AND_RET(rc)
}

static awk_value_t *
do_mdb_env_sync(int nargs, awk_value_t *result)
{
  awk_value_t force;
  MDB_env *env;
  int rc;

  if (do_lint && nargs > 2)
    lintwarn(ext_id, _("%s: called with too many arguments"), __func__+3);
  if (!(env = lookup_handle(&mdb.env, 0, NULL, awk_false, __func__+3)))
    rc = API_ERROR;
  else if (!get_argument(1, AWK_NUMBER, &force) || !is_int(&force)) {
    rc = API_ERROR;
    set_ERRNO(_("mdb_env_sync: 2rd argument must be an integer force value"));
  }
  else if ((rc = mdb_env_sync(env, force.num_value)) != MDB_SUCCESS)
    set_ERRNO(_("mdb_env_sync failed"));
  SET_AND_RET(rc)
}

static awk_value_t *
do_mdb_env_copy(int nargs, awk_value_t *result)
{
  awk_value_t path;
  MDB_env *env;
  int rc;

  if (do_lint && nargs > 2)
    lintwarn(ext_id, _("%s: called with too many arguments"), __func__+3);
  if (!(env = lookup_handle(&mdb.env, 0, NULL, awk_false, __func__+3)))
    rc = API_ERROR;
  else if (!get_argument(1, AWK_STRING, &path)) {
    rc = API_ERROR;
    set_ERRNO(_("mdb_env_copy: 2rd argument must a path string"));
  }
  else if ((rc = mdb_env_copy(env, path.str_value.str)) != MDB_SUCCESS)
    set_ERRNO(_("mdb_env_copy failed"));
  SET_AND_RET(rc)
}

static awk_value_t *
do_mdb_env_copy2(int nargs, awk_value_t *result)
{
  awk_value_t path, flags;
  MDB_env *env;
  int rc;

  if (do_lint && nargs > 2)
    lintwarn(ext_id, _("%s: called with too many arguments"), __func__+3);
  if (!(env = lookup_handle(&mdb.env, 0, NULL, awk_false, __func__+3)))
    rc = API_ERROR;
  else if (!get_argument(1, AWK_STRING, &path)) {
    rc = API_ERROR;
    set_ERRNO(_("mdb_env_copy2: 2rd argument must a path string"));
  }
  else if (!get_argument(2, AWK_NUMBER, &flags) || !is_uint(&flags)) {
    rc = API_ERROR;
    set_ERRNO(_("mdb_env_set_flags: 3rd argument must be an unsigned integer flags value"));
  }
  else if ((rc = mdb_env_copy2(env, path.str_value.str,
			       flags.num_value)) != MDB_SUCCESS)
    set_ERRNO(_("mdb_env_copy2 failed"));
  SET_AND_RET(rc)
}

static awk_value_t *
do_mdb_env_get_flags(int nargs, awk_value_t *result)
{
  int rc;
  MDB_env *env;
  unsigned int flags;

  if (do_lint && nargs > 1)
    lintwarn(ext_id, _("%s: called with too many arguments"), __func__+3);
  if (!(env = lookup_handle(&mdb.env, 0, NULL, awk_false, __func__+3))) {
    rc = API_ERROR;
    flags = 0;
  }
  else if ((rc = mdb_env_get_flags(env, &flags)) != MDB_SUCCESS) {
    set_ERRNO(_("mdb_env_get_flags failed"));
    flags = 0;
  }
  set_mdb_errno(rc);
  RET_NUM(flags);
}

static awk_value_t *
do_mdb_env_get_maxkeysize(int nargs, awk_value_t *result)
{
  MDB_env *env;

  if (do_lint && nargs > 1)
    lintwarn(ext_id, _("%s: called with too many arguments"), __func__+3);
  if (!(env = lookup_handle(&mdb.env, 0, NULL, awk_false, __func__+3))) {
    set_mdb_errno(API_ERROR);
    RET_NUM(0);
  }
  set_mdb_errno(MDB_SUCCESS);
  RET_NUM(mdb_env_get_maxkeysize(env));
}

static awk_value_t *
do_mdb_env_get_maxreaders(int nargs, awk_value_t *result)
{
  int rc;
  MDB_env *env;
  unsigned int maxreaders;

  if (do_lint && nargs > 1)
    lintwarn(ext_id, _("%s: called with too many arguments"), __func__+3);
  if (!(env = lookup_handle(&mdb.env, 0, NULL, awk_false, __func__+3))) {
    rc = API_ERROR;
    maxreaders = 0;
  }
  else if ((rc = mdb_env_get_maxreaders(env, &maxreaders)) != MDB_SUCCESS) {
    set_ERRNO(_("mdb_env_get_maxreaders failed"));
    maxreaders = 0;
  }
  set_mdb_errno(rc);
  RET_NUM(maxreaders);
}

static awk_value_t *
do_mdb_env_get_path(int nargs, awk_value_t *result)
{
  MDB_env *env;
  const char *path;

  if (do_lint && nargs > 1)
    lintwarn(ext_id, _("%s: called with too many arguments"), __func__+3);
  if (!(env = lookup_handle(&mdb.env, 0, NULL, awk_false, __func__+3))) {
    set_mdb_errno(API_ERROR);
    RET_NULSTR;
  }
  if (set_mdb_errno(mdb_env_get_path(env, &path)) != MDB_SUCCESS) {
    set_ERRNO(_("mdb_env_get_path failed"));
    RET_NULSTR;
  }
  return make_string_malloc(path, strlen(path), result);
}

static awk_value_t *
do_mdb_env_set_flags(int nargs, awk_value_t *result)
{
  awk_value_t flags, onoff;
  int rc;
  MDB_env *env;

  if (do_lint && nargs > 3)
    lintwarn(ext_id, _("%s: called with too many arguments"), __func__+3);
  if (!(env = lookup_handle(&mdb.env, 0, NULL, awk_false, __func__+3)))
    rc = API_ERROR;
  else if (!get_argument(1, AWK_NUMBER, &flags) || !is_uint(&flags)) {
    rc = API_ERROR;
    set_ERRNO(_("mdb_env_set_flags: 2nd argument must be an unsigned integer flags value"));
  }
  else if (!get_argument(2, AWK_NUMBER, &onoff) || !is_int(&onoff)) {
    rc = API_ERROR;
    set_ERRNO(_("mdb_env_set_flags: 3rd argument must be an integer onoff value"));
  }
  else if ((rc = mdb_env_set_flags(env, flags.num_value,
				   onoff.num_value)) != MDB_SUCCESS)
    set_ERRNO(_("mdb_env_set_flags failed"));
  SET_AND_RET(rc)
}

static awk_value_t *
do_mdb_env_set_mapsize(int nargs, awk_value_t *result)
{
  awk_value_t msize;
  int rc;
  MDB_env *env;

  if (do_lint && nargs > 2)
    lintwarn(ext_id, _("%s: called with too many arguments"), __func__+3);
  if (!(env = lookup_handle(&mdb.env, 0, NULL, awk_false, __func__+3)))
    rc = API_ERROR;
  else if (!get_argument(1, AWK_NUMBER, &msize) || !is_uint(&msize)) {
    rc = API_ERROR;
    set_ERRNO(_("mdb_env_set_mapsize: 2nd argument must an unsigned integer mapsize"));
  }
  else if ((rc = mdb_env_set_mapsize(env, msize.num_value)) != MDB_SUCCESS)
    set_ERRNO(_("mdb_env_set_mapsize failed"));
  SET_AND_RET(rc)
}

static awk_value_t *
do_mdb_env_set_maxdbs(int nargs, awk_value_t *result)
{
  awk_value_t dbs;
  int rc;
  MDB_env *env;

  if (do_lint && nargs > 2)
    lintwarn(ext_id, _("%s: called with too many arguments"), __func__+3);
  if (!(env = lookup_handle(&mdb.env, 0, NULL, awk_false, __func__+3)))
    rc = API_ERROR;
  else if (!get_argument(1, AWK_NUMBER, &dbs) || !is_uint(&dbs)) {
    rc = API_ERROR;
    set_ERRNO(_("mdb_env_set_maxdbs: 2nd argument must an unsigned integer number of dbs"));
  }
  else if ((rc = mdb_env_set_maxdbs(env, dbs.num_value)) != MDB_SUCCESS)
    set_ERRNO(_("mdb_env_set_maxdbs failed"));
  SET_AND_RET(rc)
}

static awk_value_t *
do_mdb_env_set_maxreaders(int nargs, awk_value_t *result)
{
  awk_value_t readers;
  int rc;
  MDB_env *env;

  if (do_lint && nargs > 2)
    lintwarn(ext_id, _("%s: called with too many arguments"), __func__+3);
  if (!(env = lookup_handle(&mdb.env, 0, NULL, awk_false, __func__+3)))
    rc = API_ERROR;
  else if (!get_argument(1, AWK_NUMBER, &readers) || !is_uint(&readers)) {
    rc = API_ERROR;
    set_ERRNO(_("mdb_env_set_maxreaders: 2nd argument must an unsigned integer number of readers"));
  }
  else if ((rc = mdb_env_set_maxreaders(env, readers.num_value)) != MDB_SUCCESS)
    set_ERRNO(_("mdb_env_set_maxreaders failed"));
  SET_AND_RET(rc)
}

static awk_value_t *
do_mdb_env_open(int nargs, awk_value_t *result)
{
  awk_value_t path, flags, mode;
  int rc;
  MDB_env *env;

  if (do_lint && nargs > 4)
    lintwarn(ext_id, _("%s: called with too many arguments"), __func__+3);
  if (!(env = lookup_handle(&mdb.env, 0, NULL, awk_false, __func__+3)))
    rc = API_ERROR;
  else if (!get_argument(1, AWK_STRING, &path)) {
    set_ERRNO(_("mdb_env_open: 2nd argument must be a string path value"));
    rc = API_ERROR;
  }
  else if (!get_argument(2, AWK_NUMBER, &flags) || !is_uint(&flags)) {
    set_ERRNO(_("mdb_env_open: 3rd argument must be an unsigned integer flags value"));
    rc = API_ERROR;
  }
  else if (!get_argument(3, AWK_NUMBER, &mode) || !is_uint(&mode)) {
    set_ERRNO(_("mdb_env_open: 4th argument must be an unsigned integer mode value"));
    rc = API_ERROR;
  }
  else if ((rc = mdb_env_open(env, path.str_value.str, flags.num_value,
			      mode.num_value)) != MDB_SUCCESS)
    set_ERRNO(_("mdb_env_open failed"));
  SET_AND_RET(rc)
}

static awk_value_t *
do_mdb_txn_begin(int nargs, awk_value_t *result)
{
  awk_value_t name, pname, flags;
  int rc;
  MDB_env *env;
  MDB_txn *parent;
  MDB_txn *txn;

  if (do_lint && nargs > 3)
    lintwarn(ext_id, _("%s: called with too many arguments"), __func__+3);
  if (!(env = lookup_handle(&mdb.env, 0, NULL, awk_false, __func__+3)))
    rc = API_ERROR;
  else if (!(parent = lookup_handle(&mdb.txn, 1, &pname, awk_true,
				    __func__+3)) && pname.str_value.len)
    rc = API_ERROR;
  else if (!get_argument(2, AWK_NUMBER, &flags) || !is_uint(&flags)) {
    set_ERRNO(_("mdb_txn_begin: 3rd argument must be an unsigned integer flags value"));
    rc = API_ERROR;
  }
  else if ((rc = mdb_txn_begin(env, parent, flags.num_value, &txn)) !=
	   MDB_SUCCESS)
    set_ERRNO(_("mdb_txn_begin failed"));
  else {
    get_handle(&mdb.txn, &name, __func__+3)->data = txn;
    set_mdb_errno(MDB_SUCCESS);
    return make_string_malloc(name.str_value.str, name.str_value.len, result);
  }
  set_mdb_errno(rc);
  RET_NULSTR;
}

static awk_value_t *
do_mdb_txn_id(int nargs, awk_value_t *result)
{
  MDB_txn *txn;

  if (do_lint && nargs > 1)
    lintwarn(ext_id, _("%s: called with too many arguments"), __func__+3);
  if (!(txn = lookup_handle(&mdb.txn, 0, NULL, awk_false, __func__+3))) {
    set_mdb_errno(API_ERROR);
    RET_NUM(0);
  }
  set_mdb_errno(MDB_SUCCESS);
  RET_NUM(mdb_txn_id(txn));
}

static awk_value_t *
do_mdb_txn_commit(int nargs, awk_value_t *result)
{
  awk_value_t name;
  MDB_txn *txn;
  int rc;

  if (do_lint && nargs > 1)
    lintwarn(ext_id, _("%s: called with too many arguments"), __func__+3);
  if (!(txn = lookup_handle(&mdb.txn, 0, &name, awk_false, __func__+3)))
    rc = API_ERROR;
  else if ((rc = mdb_txn_commit(txn)) == MDB_SUCCESS)
    release_handle(&mdb.txn, &name, __func__+3);
  else
    set_ERRNO(_("mdb_txn_commit failed"));
  SET_AND_RET(rc)
}

static awk_value_t *
do_mdb_txn_abort(int nargs, awk_value_t *result)
{
  awk_value_t name;
  MDB_txn *txn;
  int rc;

  if (do_lint && nargs > 1)
    lintwarn(ext_id, _("%s: called with too many arguments"), __func__+3);
  if (!(txn = lookup_handle(&mdb.txn, 0, &name, awk_false, __func__+3)))
    rc = API_ERROR;
  else {
    mdb_txn_abort(txn);
    release_handle(&mdb.txn, &name, __func__+3);
    rc = MDB_SUCCESS;
  }
  SET_AND_RET(rc)
}

static awk_value_t *
do_mdb_txn_reset(int nargs, awk_value_t *result)
{
  MDB_txn *txn;
  int rc;

  if (do_lint && nargs > 1)
    lintwarn(ext_id, _("%s: called with too many arguments"), __func__+3);
  if (!(txn = lookup_handle(&mdb.txn, 0, NULL, awk_false, __func__+3)))
    rc = API_ERROR;
  else {
    mdb_txn_reset(txn);
    rc = MDB_SUCCESS;
  }
  SET_AND_RET(rc)
}

static awk_value_t *
do_mdb_txn_renew(int nargs, awk_value_t *result)
{
  MDB_txn *txn;
  int rc;

  if (do_lint && nargs > 1)
    lintwarn(ext_id, _("%s: called with too many arguments"), __func__+3);
  if (!(txn = lookup_handle(&mdb.txn, 0, NULL, awk_false, __func__+3)))
    rc = API_ERROR;
  else if ((rc = mdb_txn_renew(txn)) != MDB_SUCCESS)
    set_ERRNO(_("mdb_txn_renew failed"));
  SET_AND_RET(rc)
}

static awk_value_t *
do_mdb_dbi_open(int nargs, awk_value_t *result)
{
  awk_value_t name, flags;
  int rc;
  MDB_txn *txn;
  MDB_dbi *dbi;

  if (do_lint && nargs > 3)
    lintwarn(ext_id, _("%s: called with too many arguments"), __func__+3);
  if (!(txn = lookup_handle(&mdb.txn, 0, NULL, awk_false, __func__+3)))
    rc = API_ERROR;
  else if (!get_argument(1, AWK_STRING, &name)) {
    set_ERRNO(_("mdb_dbi_open: 2nd argument must be the database name"));
    rc = API_ERROR;
  }
  else if (!get_argument(2, AWK_NUMBER, &flags) || !is_uint(&flags)) {
    set_ERRNO(_("mdb_dbi_open: 3rd argument must be an unsigned integer flags value"));
    rc = API_ERROR;
  }
  else {
    if (!(dbi = (MDB_dbi *)malloc(sizeof(*dbi))))
      fatal(ext_id, _("%s: dbi malloc failed"), __func__+3);
    if ((rc = mdb_dbi_open(txn,
			   (name.str_value.len ? name.str_value.str : NULL),
			   flags.num_value, dbi)) != MDB_SUCCESS)
      set_ERRNO(_("mdb_dbi_open failed"));
    else {
      get_handle(&mdb.dbi, &name, __func__+3)->data = dbi;
      set_mdb_errno(MDB_SUCCESS);
      return make_string_malloc(name.str_value.str, name.str_value.len, result);
    }
  }
  set_mdb_errno(rc);
  RET_NULSTR;
}

static awk_value_t *
do_mdb_dbi_close(int nargs, awk_value_t *result)
{
  awk_value_t name;
  MDB_env *env;
  MDB_dbi *dbi;
  int rc;

  if (do_lint && nargs > 2)
    lintwarn(ext_id, _("%s: called with too many arguments"), __func__+3);
  if (!(env = lookup_handle(&mdb.env, 0, NULL, awk_false, __func__+3)))
    rc = API_ERROR;
  else if (!(dbi = lookup_handle(&mdb.dbi, 1, &name, awk_false, __func__+3)))
    rc = API_ERROR;
  else {
    mdb_dbi_close(env, *dbi);
    free(dbi);
    release_handle(&mdb.dbi, &name, __func__+3);
    rc = MDB_SUCCESS;
  }
  SET_AND_RET(rc)
}

static awk_value_t *
do_mdb_dbi_flags(int nargs, awk_value_t *result)
{
  MDB_txn *txn;
  MDB_dbi *dbi;
  int rc;
  unsigned int flags;

  if (do_lint && nargs > 2)
    lintwarn(ext_id, _("%s: called with too many arguments"), __func__+3);
  if (!(txn = lookup_handle(&mdb.txn, 0, NULL, awk_false, __func__+3))) {
    rc = API_ERROR;
    flags = 0;
  }
  else if (!(dbi = lookup_handle(&mdb.dbi, 1, NULL, awk_false, __func__+3))) {
    rc = API_ERROR;
    flags = 0;
  }
  else if ((rc = mdb_dbi_flags(txn, *dbi, &flags)) != MDB_SUCCESS) {
    set_ERRNO(_("mdb_dbi_flags failed"));
    flags = 0;
  }
  set_mdb_errno(rc);
  RET_NUM(flags);
}

static awk_value_t *
do_mdb_drop(int nargs, awk_value_t *result)
{
  awk_value_t name, del;
  MDB_txn *txn;
  MDB_dbi *dbi;
  int rc;

  if (do_lint && nargs > 3)
    lintwarn(ext_id, _("%s: called with too many arguments"), __func__+3);
  if (!(txn = lookup_handle(&mdb.txn, 0, NULL, awk_false, __func__+3)))
    rc = API_ERROR;
  else if (!(dbi = lookup_handle(&mdb.dbi, 1, &name, awk_false, __func__+3)))
    rc = API_ERROR;
  else if (!get_argument(2, AWK_NUMBER, &del) ||
	   ((del.num_value != 0) && (del.num_value != 1))) {
    set_ERRNO(_("mdb_drop: 3rd argument must be 0 or 1"));
    rc = API_ERROR;
  }
  else if ((rc = mdb_drop(txn, *dbi, del.num_value)) != MDB_SUCCESS)
    set_ERRNO(_("mdb_drop failed"));
  else if (del.num_value == 1) {
    free(dbi);
    release_handle(&mdb.dbi, &name, __func__+3);
  }
  SET_AND_RET(rc)
}

static awk_value_t *
do_mdb_put(int nargs, awk_value_t *result)
{
  awk_value_t key, data, flags;
  MDB_txn *txn;
  MDB_dbi *dbi;
  int rc;

  if (do_lint && nargs > 5)
    lintwarn(ext_id, _("%s: called with too many arguments"), __func__+3);
  if (!(txn = lookup_handle(&mdb.txn, 0, NULL, awk_false, __func__+3)))
    rc = API_ERROR;
  else if (!(dbi = lookup_handle(&mdb.dbi, 1, NULL, awk_false, __func__+3)))
    rc = API_ERROR;
  else if (!get_argument(2, AWK_STRING, &key)) {
    set_ERRNO(_("mdb_put: 3rd argument must be the key string"));
    rc = API_ERROR;
  }
  else if (!get_argument(3, AWK_STRING, &data)) {
    set_ERRNO(_("mdb_put: 4th argument must be the data string"));
    rc = API_ERROR;
  }
  else if (!get_argument(4, AWK_NUMBER, &flags) || !is_uint(&flags)) {
    set_ERRNO(_("mdb_put: 5th argument must be an unsigned integer flags value"));
    rc = API_ERROR;
  }
  else {
    MDB_val mdbkey, mdbdata;

    mdbkey.mv_size = key.str_value.len;
    mdbkey.mv_data = key.str_value.str;
    mdbdata.mv_size = data.str_value.len;
    mdbdata.mv_data = data.str_value.str;
    if ((rc = mdb_put(txn, *dbi, &mdbkey, &mdbdata, flags.num_value)) !=
    	MDB_SUCCESS)
      set_ERRNO(_("mdb_put failed"));
  }
  SET_AND_RET(rc)
}

static awk_value_t *
do_mdb_get(int nargs, awk_value_t *result)
{
  awk_value_t key;
  MDB_txn *txn;
  MDB_dbi *dbi;
  int rc;

  if (do_lint && nargs > 3)
    lintwarn(ext_id, _("%s: called with too many arguments"), __func__+3);
  if (!(txn = lookup_handle(&mdb.txn, 0, NULL, awk_false, __func__+3)))
    rc = API_ERROR;
  else if (!(dbi = lookup_handle(&mdb.dbi, 1, NULL, awk_false, __func__+3)))
    rc = API_ERROR;
  else if (!get_argument(2, AWK_STRING, &key)) {
    set_ERRNO(_("mdb_get: 3rd argument must be the key string"));
    rc = API_ERROR;
  }
  else {
    MDB_val mdbkey, mdbdata;

    mdbkey.mv_size = key.str_value.len;
    mdbkey.mv_data = key.str_value.str;
    if ((rc = mdb_get(txn, *dbi, &mdbkey, &mdbdata)) == MDB_SUCCESS) {
      set_mdb_errno(MDB_SUCCESS);
      return make_string_malloc(mdbdata.mv_data, mdbdata.mv_size, result);
    }
    set_ERRNO(_("mdb_get failed"));
  }
  set_mdb_errno(rc);
  RET_NULSTR;
}

static awk_value_t *
do_mdb_del(int nargs, awk_value_t *result)
{
  awk_value_t key, data;
  MDB_txn *txn;
  MDB_dbi *dbi;
  int rc;

  if (do_lint && nargs > 4)
    lintwarn(ext_id, _("%s: called with too many arguments"), __func__+3);
  if (!(txn = lookup_handle(&mdb.txn, 0, NULL, awk_false, __func__+3)))
    rc = API_ERROR;
  else if (!(dbi = lookup_handle(&mdb.dbi, 1, NULL, awk_false, __func__+3)))
    rc = API_ERROR;
  else if (!get_argument(2, AWK_STRING, &key)) {
    set_ERRNO(_("mdb_del: 3rd argument must be the key string"));
    rc = API_ERROR;
  }
  else if ((nargs >= 4) && !get_argument(3, AWK_STRING, &data)) {
    set_ERRNO(_("mdb_del: if present, the 4th argument must be the data string"));
    rc = API_ERROR;
  }
  else {
    MDB_val mdbkey, mdbdata;
    MDB_val *dp;

    mdbkey.mv_size = key.str_value.len;
    mdbkey.mv_data = key.str_value.str;
    if (nargs < 4)
      dp = NULL;
    else {
      mdbdata.mv_size = data.str_value.len;
      mdbdata.mv_data = data.str_value.str;
      dp = &mdbdata;
    }
    if ((rc = mdb_del(txn, *dbi, &mdbkey, dp)) != MDB_SUCCESS)
      set_ERRNO(_("mdb_del failed"));
  }
  SET_AND_RET(rc)
}

static awk_value_t *
do_mdb_cursor_open(int nargs, awk_value_t *result)
{
  MDB_txn *txn;
  MDB_dbi *dbi;
  MDB_cursor *cursor;
  int rc;

  if (do_lint && nargs > 2)
    lintwarn(ext_id, _("%s: called with too many arguments"), __func__+3);
  if (!(txn = lookup_handle(&mdb.txn, 0, NULL, awk_false, __func__+3)))
    rc = API_ERROR;
  else if (!(dbi = lookup_handle(&mdb.dbi, 1, NULL, awk_false, __func__+3)))
    rc = API_ERROR;
  else if ((rc = mdb_cursor_open(txn, *dbi, &cursor)) != MDB_SUCCESS)
    set_ERRNO(_("mdb_cursor_open failed"));
  else {
    awk_value_t name;

    get_handle(&mdb.cursor, &name, __func__+3)->data = cursor;
    set_mdb_errno(MDB_SUCCESS);
    return make_string_malloc(name.str_value.str, name.str_value.len, result);
  }
  set_mdb_errno(rc);
  RET_NULSTR;
}

static awk_value_t *
do_mdb_cursor_close(int nargs, awk_value_t *result)
{
  awk_value_t name;
  MDB_cursor *cursor;
  int rc;

  if (do_lint && nargs > 1)
    lintwarn(ext_id, _("%s: called with too many arguments"), __func__+3);
  if (!(cursor = lookup_handle(&mdb.cursor, 0, &name, awk_false, __func__+3)))
    rc = API_ERROR;
  else {
    mdb_cursor_close(cursor);
    release_handle(&mdb.cursor, &name, __func__+3);
    rc = MDB_SUCCESS;
  }
  SET_AND_RET(rc)
}

static awk_value_t *
do_mdb_cursor_renew(int nargs, awk_value_t *result)
{
  MDB_txn *txn;
  MDB_cursor *cursor;
  int rc;

  if (do_lint && nargs > 2)
    lintwarn(ext_id, _("%s: called with too many arguments"), __func__+3);
  if (!(txn = lookup_handle(&mdb.txn, 0, NULL, awk_false, __func__+3)))
    rc = API_ERROR;
  else if (!(cursor = lookup_handle(&mdb.cursor, 1, NULL, awk_false,
				    __func__+3)))
    rc = API_ERROR;
  else if ((rc = mdb_cursor_renew(txn, cursor)) != MDB_SUCCESS)
    set_ERRNO(_("mdb_cursor_renew failed"));
  SET_AND_RET(rc)
}

static awk_value_t *
do_mdb_cursor_put(int nargs, awk_value_t *result)
{
  awk_value_t key, data, flags;
  MDB_cursor *cursor;
  int rc;

  if (do_lint && nargs > 4)
    lintwarn(ext_id, _("%s: called with too many arguments"), __func__+3);
  if (!(cursor = lookup_handle(&mdb.cursor, 0, NULL, awk_false, __func__+3)))
    rc = API_ERROR;
  else if (!get_argument(1, AWK_STRING, &key)) {
    set_ERRNO(_("mdb_cursor_put: 2nd argument must be the key string"));
    rc = API_ERROR;
  }
  else if (!get_argument(2, AWK_STRING, &data)) {
    set_ERRNO(_("mdb_cursor_put: 3rd argument must be the data string"));
    rc = API_ERROR;
  }
  else if (!get_argument(3, AWK_NUMBER, &flags) || !is_uint(&flags)) {
    set_ERRNO(_("mdb_cursor_put: 4th argument must be an unsigned integer flags value"));
    rc = API_ERROR;
  }
  else {
    MDB_val mdbkey, mdbdata;

    mdbkey.mv_size = key.str_value.len;
    mdbkey.mv_data = key.str_value.str;
    mdbdata.mv_size = data.str_value.len;
    mdbdata.mv_data = data.str_value.str;
    if ((rc = mdb_cursor_put(cursor, &mdbkey, &mdbdata, flags.num_value)) !=
    	MDB_SUCCESS)
      set_ERRNO(_("mdb_cursor_put failed"));
  }
  SET_AND_RET(rc)
}

static awk_value_t *
do_mdb_cursor_del(int nargs, awk_value_t *result)
{
  awk_value_t flags;
  MDB_cursor *cursor;
  int rc;

  if (do_lint && nargs > 2)
    lintwarn(ext_id, _("%s: called with too many arguments"), __func__+3);
  if (!(cursor = lookup_handle(&mdb.cursor, 0, NULL, awk_false, __func__+3)))
    rc = API_ERROR;
  else if (!get_argument(1, AWK_NUMBER, &flags) || !is_uint(&flags)) {
    set_ERRNO(_("mdb_cursor_del: 2nd argument must be an unsigned integer flags value"));
    rc = API_ERROR;
  }
  else if ((rc = mdb_cursor_del(cursor, flags.num_value)) != MDB_SUCCESS)
    set_ERRNO(_("mdb_cursor_del failed"));
  SET_AND_RET(rc)
}

static awk_value_t *
do_mdb_cursor_count(int nargs, awk_value_t *result)
{
  MDB_cursor *cursor;
  size_t count;
  int rc;

  if (do_lint && nargs > 1)
    lintwarn(ext_id, _("%s: called with too many arguments"), __func__+3);
  if (!(cursor = lookup_handle(&mdb.cursor, 0, NULL, awk_false, __func__+3))) {
    rc = API_ERROR;
    count = 0;
  }
  else if ((rc = mdb_cursor_count(cursor, &count)) != MDB_SUCCESS) {
    set_ERRNO(_("mdb_cursor_count failed"));
    count = 0;
  }
  set_mdb_errno(rc);
  RET_NUM(count);
}

static awk_value_t *
do_mdb_cursor_get(int nargs, awk_value_t *result)
{
  awk_value_t arr, op;
  MDB_cursor *cursor;
  int rc;

  if (do_lint && nargs > 3)
    lintwarn(ext_id, _("%s: called with too many arguments"), __func__+3);
  if (!(cursor = lookup_handle(&mdb.cursor, 0, NULL, awk_false, __func__+3)))
    rc = API_ERROR;
  else if (!get_argument(1, AWK_ARRAY, &arr)) {
    set_ERRNO(_("mdb_cursor_get: 2nd argument must be an array"));
    rc = API_ERROR;
  }
  else if (!get_argument(2, AWK_NUMBER, &op) || !is_uint(&op)) {
    set_ERRNO(_("mdb_cursor_get: 3rd argument must be an unsigned integer cursor op"));
    rc = API_ERROR;
  }
  else {
    MDB_val mdbkey, mdbdata;
    {
      awk_value_t x;
      if (get_array_element(arr.array_cookie, &ksub, AWK_STRING, &x)) {
	mdbkey.mv_size = x.str_value.len;
	mdbkey.mv_data = x.str_value.str;
      }
      else {
	mdbkey.mv_size = 0;
	mdbkey.mv_data = NULL;
      }
    }
    {
      awk_value_t x;
      if (get_array_element(arr.array_cookie, &dsub, AWK_STRING, &x)) {
	mdbdata.mv_size = x.str_value.len;
	mdbdata.mv_data = x.str_value.str;
      }
      else {
	mdbdata.mv_size = 0;
	mdbdata.mv_data = NULL;
      }
    }
    if ((rc = mdb_cursor_get(cursor, &mdbkey, &mdbdata,
			     op.num_value)) != MDB_SUCCESS)
      set_ERRNO(_("mdb_cursor_get failed"));
    else {
      awk_value_t x;
      if (!set_array_element(arr.array_cookie, &ksub,
			     make_string_malloc(mdbkey.mv_data, mdbkey.mv_size,
			     			&x))) {
	set_ERRNO(_("mdb_cursor_get: cannot populate key array element"));
	rc = API_ERROR;
      }
      else if (!set_array_element(arr.array_cookie, &dsub,
				  make_string_malloc(mdbdata.mv_data,
						     mdbdata.mv_size,
						     &x))) {
	set_ERRNO(_("mdb_cursor_get: cannot populate data array element"));
	rc = API_ERROR;
      }
    }
  }
  SET_AND_RET(rc)
}

static awk_value_t *
do_mdb_reader_check(int nargs, awk_value_t *result)
{
  MDB_env *env;
  int rc, dead;

  if (do_lint && nargs > 1)
    lintwarn(ext_id, _("%s: called with too many arguments"), __func__+3);
  if (!(env = lookup_handle(&mdb.env, 0, NULL, awk_false, __func__+3))) {
    rc = API_ERROR;
    dead = 0;
  }
  else if ((rc = mdb_reader_check(env, &dead)) != MDB_SUCCESS) {
    dead = 0;
    set_ERRNO(_("mdb_reader_check failed"));
  }
  set_mdb_errno(rc);
  RET_NUM(dead);
}

typedef int (*mdb_cmp_func_p)(MDB_txn *txn, MDB_dbi dbi,
			      const MDB_val *a, const MDB_val *b);

static awk_value_t *
cmp_backend(int nargs, awk_value_t *result, mdb_cmp_func_p cfunc,
	    const char *funcname)
{
  awk_value_t a, b;
  MDB_txn *txn;
  MDB_dbi *dbi;
  int rc;

  if (do_lint && nargs > 4)
    lintwarn(ext_id, _("%s: called with too many arguments"), funcname);
  if (!(txn = lookup_handle(&mdb.txn, 0, NULL, awk_false, funcname)))
    rc = API_ERROR;
  else if (!(dbi = lookup_handle(&mdb.dbi, 1, NULL, awk_false, funcname)))
    rc = API_ERROR;
  else if (!get_argument(2, AWK_STRING, &a)) {
    set_ERRNO(_("mdb_cmp: 3rd argument must be a string"));
    rc = API_ERROR;
  }
  else if (!get_argument(3, AWK_STRING, &b)) {
    set_ERRNO(_("mdb_cmp: 4th argument must be a string"));
    rc = API_ERROR;
  }
  else {
    MDB_val ma, mb;

    ma.mv_size = a.str_value.len;
    ma.mv_data = a.str_value.str;
    mb.mv_size = b.str_value.len;
    mb.mv_data = b.str_value.str;
    set_mdb_errno(MDB_SUCCESS);
    RET_NUM((*cfunc)(txn, *dbi, &ma, &mb));
  }
  set_mdb_errno(rc);
  RET_NUM(0);
}

static awk_value_t *
do_mdb_cmp(int nargs, awk_value_t *result)
{
  return cmp_backend(nargs, result, mdb_cmp, __func__+3);
}

static awk_value_t *
do_mdb_dcmp(int nargs, awk_value_t *result)
{
  return cmp_backend(nargs, result, mdb_dcmp, __func__+3);
}

static awk_ext_func_t func_table[] = {
  { "mdb_strerror", do_mdb_strerror, 1 },
  { "mdb_env_create", do_mdb_env_create, 0 },
  { "mdb_env_get_flags", do_mdb_env_get_flags, 1 },
  { "mdb_env_get_maxkeysize", do_mdb_env_get_maxkeysize, 1 },
  { "mdb_env_get_maxreaders", do_mdb_env_get_maxreaders, 1 },
  { "mdb_env_get_path", do_mdb_env_get_path, 1 },
  { "mdb_env_set_flags", do_mdb_env_set_flags, 3 },
  { "mdb_env_set_mapsize", do_mdb_env_set_mapsize, 2 },
  { "mdb_env_set_maxdbs", do_mdb_env_set_maxdbs, 2 },
  { "mdb_env_set_maxreaders", do_mdb_env_set_maxreaders, 2 },
  { "mdb_env_open", do_mdb_env_open, 4 },
  { "mdb_env_close", do_mdb_env_close, 1 },
  { "mdb_env_sync", do_mdb_env_sync, 2 },
  { "mdb_env_copy", do_mdb_env_copy, 2 },
  { "mdb_env_copy2", do_mdb_env_copy2, 3 },
  { "mdb_txn_begin", do_mdb_txn_begin, 3 },
  { "mdb_txn_id", do_mdb_txn_id, 1 },
  { "mdb_txn_commit", do_mdb_txn_commit, 1 },
  { "mdb_txn_abort", do_mdb_txn_abort, 1 },
  { "mdb_txn_reset", do_mdb_txn_reset, 1 },
  { "mdb_txn_renew", do_mdb_txn_renew, 1 },
  { "mdb_dbi_open", do_mdb_dbi_open, 3 },
  { "mdb_dbi_close", do_mdb_dbi_close, 2 },
  { "mdb_dbi_flags", do_mdb_dbi_flags, 2 },
  { "mdb_drop", do_mdb_drop, 3 },
  { "mdb_put", do_mdb_put, 5 },
  { "mdb_get", do_mdb_get, 3 },
  { "mdb_del", do_mdb_del, 4 },
  { "mdb_cursor_open", do_mdb_cursor_open, 2 },
  { "mdb_cursor_close", do_mdb_cursor_close, 1 },
  { "mdb_cursor_renew", do_mdb_cursor_renew, 2 },
  { "mdb_cursor_put", do_mdb_cursor_put, 4 },
  { "mdb_cursor_del", do_mdb_cursor_del, 2 },
  { "mdb_cursor_count", do_mdb_cursor_count, 1 },
  { "mdb_cursor_get", do_mdb_cursor_get, 3 },
  { "mdb_reader_check", do_mdb_reader_check, 1 },
  { "mdb_cmp", do_mdb_cmp, 4 },
  { "mdb_dcmp", do_mdb_dcmp, 4 },
  { "mdb_version", do_mdb_version, 1 },
};

static awk_bool_t
init_my_module(void)
{
  GAWKEXTLIB_COMMON_INIT

  assert(MDB_SUCCESS != API_ERROR);	/* sanity check */
  /* strhash_create exits on failure, so no need to check return code */
  mdb.env.ht = strhash_create(0);
  mdb.txn.ht = strhash_create(0);
  mdb.dbi.ht = strhash_create(0);
  mdb.cursor.ht = strhash_create(0);
  {
    awk_scalar_t cookie;

    make_number(MDB_SUCCESS, &mdb_errno.value);
    if (!gawk_varinit_constant("MDB_SUCCESS", &mdb_errno.value, &cookie))
      fatal(ext_id, _("lmdb: unable to initialize MDB_SUCCESS"));
    if (!gawk_varinit_scalar("MDB_ERRNO", &mdb_errno.value, awk_true,
			     &mdb_errno.cookie))
      fatal(ext_id, _("lmdb: unable to initialize MDB_ERRNO"));
  }
  {
    /* check version */
    int major, minor;
    const char *verstr = mdb_version(&major, &minor, NULL);
    if ((MDB_VERSION_MAJOR != major) || (MDB_VERSION_MINOR > minor))
      fatal(ext_id,
	    _("lmdb compile-time version `%s' inconsistent with run-time "
	      "library version `%s'"), MDB_VERSION_STRING, verstr);
  }
  {
    awk_array_t mc;
    size_t i;

    if (!gawk_varinit_array("MDB", 1, &mc))
      fatal(ext_id, _("lmdb: unable to create MDB array"));
    for (i = 0; i < sizeof(mdbdef)/sizeof(mdbdef[0]); i++) {
      awk_value_t idx, val;
      if (!set_array_element(mc,
			     make_string_malloc(mdbdef[i].name,
			     			strlen(mdbdef[i].name), &idx),
			     make_number(mdbdef[i].val, &val)))
	fatal(ext_id, _("lmdb: unable to initialize MDB[%s]"), mdbdef[i].name);
    }
  }
  /* create value cookies for mdb_cursor_get subscripts */
  {
    awk_value_t x;
    if (!create_value(make_number(0, &x), &ksub.value_cookie))
      fatal(ext_id, _("lmdb: unable to create key subscript value"));
    if (!create_value(make_number(1, &x), &dsub.value_cookie))
      fatal(ext_id, _("lmdb: unable to create data subscript value"));
    ksub.val_type = dsub.val_type = AWK_VALUE_COOKIE;
  }

  return awk_true;
}

static awk_bool_t (*init_func)(void) = init_my_module;
static const char *ext_version = PACKAGE_STRING;

dl_load_func(func_table, lmdb, "")
