#include <iostream>
#include <string>
#include <hiredis/hiredis.h>

int main() {
    // Connect to Redis
    redisContext* ctx = redisConnect("127.0.0.1", 6379);
    if (ctx == nullptr || ctx->err) {
        std::cerr << "Error connecting to Redis: "
                  << (ctx ? ctx->errstr : "null context") << std::endl;
        return 1;
    }
    std::cout << "Connected to Redis!" << std::endl;

    // SET a value
    redisReply* reply = (redisReply*)redisCommand(ctx, "SET safemon:status running");
    std::cout << "SET: " << reply->str << std::endl;
    freeReplyObject(reply);

    // GET it back
    reply = (redisReply*)redisCommand(ctx, "GET safemon:status");
    std::cout << "GET safemon:status: " << reply->str << std::endl;
    freeReplyObject(reply);

    redisFree(ctx);
    return 0;
}