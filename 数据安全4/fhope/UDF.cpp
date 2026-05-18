#include "Node.h"

#include <cstdio>
#include <cstring>
#include <mysql/mysql.h>
#include <string>

#ifndef MYSQL_ERRMSG_SIZE
#define MYSQL_ERRMSG_SIZE 512
#endif

namespace
{
bool require_args(UDF_ARGS *args, unsigned int count, char *message)
{
    if (args->arg_count != count)
    {
        std::snprintf(message, MYSQL_ERRMSG_SIZE, "expected %u arguments", count);
        return true;
    }
    return false;
}

long long get_int_arg(UDF_ARGS *args, unsigned int index)
{
    if (!args->args[index])
    {
        return 0;
    }
    return *reinterpret_cast<long long *>(args->args[index]);
}

std::string get_string_arg(UDF_ARGS *args, unsigned int index)
{
    if (!args->args[index])
    {
        return "";
    }
    return std::string(args->args[index], args->lengths[index]);
}

void ensure_root()
{
    if (root == nullptr)
    {
        root_initial();
    }
}
} // namespace

extern "C"
{
    bool FHInsert_init(UDF_INIT *initid, UDF_ARGS *args, char *message);
    long long FHInsert(UDF_INIT *initid, UDF_ARGS *args, char *is_null, char *error);

    bool FHSearch_init(UDF_INIT *initid, UDF_ARGS *args, char *message);
    long long FHSearch(UDF_INIT *initid, UDF_ARGS *args, char *is_null, char *error);

    bool FHUpdate_init(UDF_INIT *initid, UDF_ARGS *args, char *message);
    long long FHUpdate(UDF_INIT *initid, UDF_ARGS *args, char *is_null, char *error);

    bool FHStart_init(UDF_INIT *initid, UDF_ARGS *args, char *message);
    long long FHStart(UDF_INIT *initid, UDF_ARGS *args, char *is_null, char *error);

    bool FHEnd_init(UDF_INIT *initid, UDF_ARGS *args, char *message);
    long long FHEnd(UDF_INIT *initid, UDF_ARGS *args, char *is_null, char *error);

    bool FHReset_init(UDF_INIT *initid, UDF_ARGS *args, char *message);
    long long FHReset(UDF_INIT *initid, UDF_ARGS *args, char *is_null, char *error);

    bool FHTotalCount_init(UDF_INIT *initid, UDF_ARGS *args, char *message);
    long long FHTotalCount(UDF_INIT *initid, UDF_ARGS *args, char *is_null, char *error);

    bool FHHeight_init(UDF_INIT *initid, UDF_ARGS *args, char *message);
    long long FHHeight(UDF_INIT *initid, UDF_ARGS *args, char *is_null, char *error);

    bool FHLeafCount_init(UDF_INIT *initid, UDF_ARGS *args, char *message);
    long long FHLeafCount(UDF_INIT *initid, UDF_ARGS *args, char *is_null, char *error);

    bool FHMaxLeafSize_init(UDF_INIT *initid, UDF_ARGS *args, char *message);
    long long FHMaxLeafSize(UDF_INIT *initid, UDF_ARGS *args, char *is_null, char *error);
}

bool FHInsert_init(UDF_INIT *initid, UDF_ARGS *args, char *message)
{
    if (require_args(args, 2, message))
    {
        return true;
    }
    args->arg_type[0] = INT_RESULT;
    args->arg_type[1] = STRING_RESULT;
    return false;
}

long long FHInsert(UDF_INIT *initid, UDF_ARGS *args, char *is_null, char *error)
{
    ensure_root();
    start_update = -1;
    end_update = -1;
    update.clear();

    int pos = static_cast<int>(get_int_arg(args, 0));
    std::string cipher = get_string_arg(args, 1);
    return root->insert(pos, cipher);
}

bool FHSearch_init(UDF_INIT *initid, UDF_ARGS *args, char *message)
{
    if (require_args(args, 1, message))
    {
        return true;
    }
    args->arg_type[0] = INT_RESULT;
    return false;
}

long long FHSearch(UDF_INIT *initid, UDF_ARGS *args, char *is_null, char *error)
{
    ensure_root();
    int pos = static_cast<int>(get_int_arg(args, 0));
    if (pos < 0)
    {
        return 0;
    }
    return root->search(pos);
}

bool FHUpdate_init(UDF_INIT *initid, UDF_ARGS *args, char *message)
{
    if (require_args(args, 1, message))
    {
        return true;
    }
    args->arg_type[0] = STRING_RESULT;
    return false;
}

long long FHUpdate(UDF_INIT *initid, UDF_ARGS *args, char *is_null, char *error)
{
    std::string cipher = get_string_arg(args, 0);
    return get_update(cipher);
}

bool FHStart_init(UDF_INIT *initid, UDF_ARGS *args, char *message)
{
    return require_args(args, 0, message);
}

long long FHStart(UDF_INIT *initid, UDF_ARGS *args, char *is_null, char *error)
{
    return start_update;
}

bool FHEnd_init(UDF_INIT *initid, UDF_ARGS *args, char *message)
{
    return require_args(args, 0, message);
}

long long FHEnd(UDF_INIT *initid, UDF_ARGS *args, char *is_null, char *error)
{
    return end_update;
}

bool FHReset_init(UDF_INIT *initid, UDF_ARGS *args, char *message)
{
    return require_args(args, 0, message);
}

long long FHReset(UDF_INIT *initid, UDF_ARGS *args, char *is_null, char *error)
{
    root_initial();
    return 1;
}

bool FHTotalCount_init(UDF_INIT *initid, UDF_ARGS *args, char *message)
{
    return require_args(args, 0, message);
}

long long FHTotalCount(UDF_INIT *initid, UDF_ARGS *args, char *is_null, char *error)
{
    ensure_root();
    return tree_stats().total_count;
}

bool FHHeight_init(UDF_INIT *initid, UDF_ARGS *args, char *message)
{
    return require_args(args, 0, message);
}

long long FHHeight(UDF_INIT *initid, UDF_ARGS *args, char *is_null, char *error)
{
    ensure_root();
    return tree_stats().height;
}

bool FHLeafCount_init(UDF_INIT *initid, UDF_ARGS *args, char *message)
{
    return require_args(args, 0, message);
}

long long FHLeafCount(UDF_INIT *initid, UDF_ARGS *args, char *is_null, char *error)
{
    ensure_root();
    return tree_stats().leaf_count;
}

bool FHMaxLeafSize_init(UDF_INIT *initid, UDF_ARGS *args, char *message)
{
    return require_args(args, 0, message);
}

long long FHMaxLeafSize(UDF_INIT *initid, UDF_ARGS *args, char *is_null, char *error)
{
    ensure_root();
    return tree_stats().max_leaf_size;
}
