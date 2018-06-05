#include "postgres.h"
#include "fmgr.h"
#include "utils/builtins.h"
#include "utils/guc.h"
#include <string.h>
#include <hiredis/hiredis.h>

#ifdef PG_MODULE_MAGIC
PG_MODULE_MAGIC;
#endif

void _PG_init(void);

/**
 * Redis Context
**/
static redisContext *ctx;
char *redisHost = "127.0.0.1";
int redisPort = 6379;


/**
 * Return the connection status.
**/
PG_FUNCTION_INFO_V1(redis_status);
extern Datum redis_status(PG_FUNCTION_ARGS);
Datum
redis_status(PG_FUNCTION_ARGS)
{
    char msg[1024];

    if (ctx == NULL) {
        sprintf(msg, "redis://%s:%d Not Connected", redisHost, redisPort);
        PG_RETURN_TEXT_P(cstring_to_text(msg));
    }

    if (ctx->err) {
        sprintf(msg, "redis://%s:%d Error: %s", redisHost, redisPort, ctx->errstr);
        PG_RETURN_TEXT_P(cstring_to_text(msg));
    }

    sprintf(msg, "redis://%s:%d Connected", ctx->tcp.host, ctx->tcp.port);
    PG_RETURN_TEXT_P(cstring_to_text(msg));
}

/**
 * Connect
**/
PG_FUNCTION_INFO_V1(redis_connect);
extern Datum redis_connect(PG_FUNCTION_ARGS);
Datum
redis_connect(PG_FUNCTION_ARGS)
{
    if (ctx == NULL || ctx->err) {
        ctx = redisConnect(redisHost, redisPort);
        if (ctx == NULL || ctx->err) {
            ereport(ERROR, (errcode(ERRCODE_INVALID_PARAMETER_VALUE), errmsg("Cannot connect to redis://%s:%d.", redisHost, redisPort)));
        }
    }

    PG_RETURN_BOOL(true);
}

/**
 * Disconnect
**/
PG_FUNCTION_INFO_V1(redis_disconnect);
extern Datum redis_disconnect(PG_FUNCTION_ARGS);
Datum
redis_disconnect(PG_FUNCTION_ARGS)
{
    if (ctx != NULL) {
        redisFree(ctx);
        ctx = NULL;
    }

    PG_RETURN_BOOL(true);
}

/**
 * Publish a message to the channel.
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

    redisReply *reply;

    if (ctx == NULL || ctx->err) {
        ctx = redisConnect(redisHost, redisPort);
        if (ctx == NULL || ctx->err) {
            ereport(ERROR, (errcode(ERRCODE_INVALID_PARAMETER_VALUE), errmsg("Cannot connect to redis://%s:%d.", redisHost, redisPort)));
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

    PG_RETURN_BOOL(true);
}

/**
 * Initialize
**/
void
_PG_init(void)
{
    static bool init = false;

    if (init) {
        return;
    }

    DefineCustomStringVariable(
        "redis.host",
        "Redis Host",
        NULL,
        &redisHost,
        "127.0.0.1",
        PGC_USERSET,
        GUC_NOT_IN_SAMPLE,
        NULL,
        NULL,
        NULL
    );

    DefineCustomIntVariable(
        "redis.port",
        "Redis Port",
        NULL,
        &redisPort,
        6379,
        1,
        65535,
        PGC_USERSET,
        GUC_NOT_IN_SAMPLE,
        NULL,
        NULL,
        NULL
    );

    init = true;
}