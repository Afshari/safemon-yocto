#include "redis_client_impl.h"
#include <iostream>

RedisClient::RedisClient(const std::string& host, int port)
{
    redis_ = redisConnect(host.c_str(), port);
    if (!redis_ || redis_->err)
        std::cerr << "[redis_client] Connection failed\n";
    else
        std::cout << "[redis_client] Connected\n";
}

RedisClient::~RedisClient()
{
    if (redis_) redisFree(redis_);
}

std::string RedisClient::get_latest_frame()
{
    redisReply* reply = (redisReply*)redisCommand(
        redis_, "LINDEX safemon:can:frames 0");
    std::string result;
    if (reply && reply->type == REDIS_REPLY_STRING)
        result = std::string(reply->str, reply->len);
    if (reply) freeReplyObject(reply);
    return result;
}

void RedisClient::publish_fault(const std::string& level,
                                 const std::string& message)
{
    std::string fault = level + ":" + message;
    redisReply* reply = (redisReply*)redisCommand(
        redis_, "SET safemon:faults:current %s", fault.c_str());
    if (reply) freeReplyObject(reply);
}