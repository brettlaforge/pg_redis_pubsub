#include "postgres.h"
#include "fmgr.h"
#include "utils/builtins.h"
#include <hiredis/hiredis.h>
PG_MODULE_MAGIC;

/**
 * Redis Context
**/
static redisContext *ctx;

/**
 * Return the connection status.
**/
PG_FUNCTION_INFO_V1(redis_status);
extern Datum redis_status(PG_FUNCTION_ARGS);
Datum
redis_status(PG_FUNCTION_ARGS)
{
    if (ctx == NULL) {
        PG_RETURN_TEXT_P(cstring_to_text("Disconnected"));
    }
    if (ctx->err) {
        PG_RETURN_TEXT_P(cstring_to_text(strcat("Connection Error: ", ctx->errstr)));
    }
    PG_RETURN_TEXT_P(cstring_to_text("Connected"));
}

/**
 * Posts a message to the given channel.
**/
PG_FUNCTION_INFO_V1(redis_publish);
extern Datum redis_publish(PG_FUNCTION_ARGS);
Datum
redis_publish(PG_FUNCTION_ARGS)
{
    text *pchannel = PG_GETARG_TEXT_P(0);
    text *pmessage = PG_GETARG_TEXT_P(1);
    char *channel = text_to_cstring(pchannel);
    char *message = text_to_cstring(pmessage);
    char *host = "127.0.0.1";
    int port = 6379;
    redisReply *reply;
    if (ctx == NULL || ctx->err) {
        ctx = redisConnect(host, port);
        if (ctx == NULL || ctx->err) {
            ereport(ERROR, (errcode(ERRCODE_INVALID_PARAMETER_VALUE), errmsg("Cannot connect to Redis.")));
        }
    }
    reply = redisCommand(ctx, "PUBLISH %s %s", channel, message);
    if (reply == NULL) {
        char *err = pstrdup(ctx->errstr);
        redisFree(ctx);
        ereport(ERROR, (errcode(ERRCODE_INVALID_PARAMETER_VALUE), errmsg("%s", err)));
    }
    if (reply->type == REDIS_REPLY_ERROR) {
        char *err = pstrdup(reply->str);
        freeReplyObject(reply);
        ereport(ERROR, (errcode(ERRCODE_INVALID_PARAMETER_VALUE), errmsg("%s", err)));
    }
    PG_RETURN_VOID();
}