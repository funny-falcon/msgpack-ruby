/*
 * MessagePack for Ruby
 *
 * Copyright (C) 2008-2012 FURUHASHI Sadayuki
 *
 *    Licensed under the Apache License, Version 2.0 (the "License");
 *    you may not use this file except in compliance with the License.
 *    You may obtain a copy of the License at
 *
 *        http://www.apache.org/licenses/LICENSE-2.0
 *
 *    Unless required by applicable law or agreed to in writing, software
 *    distributed under the License is distributed on an "AS IS" BASIS,
 *    WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 *    See the License for the specific language governing permissions and
 *    limitations under the License.
 */
#ifndef MSGPACK_RUBY_POOL_H__
#define MSGPACK_RUBY_POOL_H__

#include "premem.h"
#include "postmem.h"

struct msgpack_pool_t;
typedef struct msgpack_pool_t msgpack_pool_t;

struct msgpack_pool_t {
    msgpack_premem_t premem;
    msgpack_postmem_t postmem;
};

static inline void msgpack_pool_init(msgpack_pool_t* pl,
        size_t pre_size, size_t post_size, size_t pool_size)
{
    msgpack_premem_init(&pl->premem, pre_size);
    msgpack_postmem_init(&pl->postmem, post_size, pool_size);
}

#ifndef MSGPACK_POSTMEM_CHUNK_SIZE
#define MSGPACK_POSTMEM_CHUNK_SIZE 32*1024
#endif

#ifndef MSGPACK_POSTMEM_SIZE
#define MSGPACK_POSTMEM_SIZE 64
#endif

#ifndef MSGPACK_PREMEM_SIZE
#define MSGPACK_PREMEM_SIZE 1024
#endif

// TODO
//#if MSGPACK_PREMEM_SIZE < (RSTRING_EMBED_LEN_MAX)
//#undef MSGPACK_PREMEM_SIZE
//#define MSGPACK_PREMEM_SIZE RSTRING_EMBED_LEN_MAX
//#endif

static inline void msgpack_pool_init_default(msgpack_pool_t* pl)
{
    // TODO
    msgpack_pool_init(pl, MSGPACK_PREMEM_SIZE, MSGPACK_POSTMEM_CHUNK_SIZE, MSGPACK_POSTMEM_SIZE);
}

static inline void msgpack_pool_destroy(msgpack_pool_t* pl)
{
    msgpack_premem_destroy(&pl->premem);
    msgpack_postmem_destroy(&pl->postmem);
}

static inline void* msgpack_pool_alloc(msgpack_pool_t* pl,
        size_t required_size, size_t* allocated_size)
{
    if(required_size <= pl->premem.alloc_size) {
        *allocated_size = pl->premem.alloc_size;
        return msgpack_premem_alloc(&pl->premem);
    }
    return msgpack_postmem_alloc(&pl->postmem, required_size, allocated_size);
}

static inline void* msgpack_pool_realloc(msgpack_pool_t* pl,
        void* ptr, size_t required_size, size_t* current_size)
{
    size_t len = *current_size;
    if(len <= pl->premem.alloc_size) {
        void* ptr2 = msgpack_postmem_alloc(&pl->postmem, required_size, current_size);
        memcpy(ptr2, ptr, len);
        msgpack_premem_free(&pl->premem, ptr);
        return ptr2;
    }
    return msgpack_postmem_realloc(&pl->postmem, ptr, required_size, current_size);
}

static inline void msgpack_pool_free(msgpack_pool_t* pl,
        void* ptr, size_t min_size)
{
    if(min_size <= pl->premem.alloc_size) {
        if(msgpack_premem_free(&pl->premem, ptr)) {
            return;
        }
    }
    msgpack_postmem_free(&pl->postmem, ptr, min_size);
}

static inline bool msgpack_pool_try_move_to_string(msgpack_pool_t* pl,
        void* ptr, size_t size, VALUE* to)
{
    if(size < pl->premem.alloc_size) {
        return false;
    }
#ifdef USE_STR_NEW_MOVE
    *to = msgpack_postmem_move_to_string(&pl->postmem, ptr, size);
    return true;
#else
    return false;
#endif
}


extern msgpack_pool_t msgpack_pool_static_instance;

static inline void msgpack_pool_static_init(
        size_t pre_size, size_t post_size, size_t pool_size)
{
    msgpack_pool_init(&msgpack_pool_static_instance,
            pre_size, post_size, pool_size);
}

static inline void msgpack_pool_static_init_default()
{
    msgpack_pool_init_default(&msgpack_pool_static_instance);
}

static inline void msgpack_pool_static_destroy()
{
    msgpack_pool_destroy(&msgpack_pool_static_instance);
}

static inline void* msgpack_pool_static_malloc(
        size_t required_size, size_t* allocated_size)
{
    return msgpack_pool_alloc(&msgpack_pool_static_instance, required_size, allocated_size);
}

static inline void* msgpack_pool_static_realloc(
        void* ptr, size_t required_size, size_t* current_size)
{
    return msgpack_pool_realloc(&msgpack_pool_static_instance,
            ptr, required_size, current_size);
}

static inline void msgpack_pool_static_free(
        void* ptr, size_t min_size)
{
    return msgpack_pool_free(&msgpack_pool_static_instance,
            ptr, min_size);
}

static inline bool msgpack_pool_static_try_move_to_string(
        void* ptr, size_t size, VALUE* to)
{
    return msgpack_pool_try_move_to_string(&msgpack_pool_static_instance,
            ptr, size, to);
}

#endif

