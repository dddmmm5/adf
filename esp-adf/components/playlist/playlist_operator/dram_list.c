/*
 * ESPRESSIF MIT License
 *
 * Copyright (c) 2019 <ESPRESSIF SYSTEMS (SHANGHAI) CO., LTD>
 *
 * Permission is hereby granted for use on all ESPRESSIF SYSTEMS products, in which case,
 * it is free of charge, to any person obtaining a copy of this software and associated
 * documentation files (the "Software"), to deal in the Software without restriction, including
 * without limitation the rights to use, copy, modify, merge, publish, distribute, sublicense,
 * and/or sell copies of the Software, and to permit persons to whom the Software is furnished
 * to do so, subject to the following conditions:
 *
 * The above copyright notice and this permission notice shall be included in all copies or
 * substantial portions of the Software.
 *
 * THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
 * IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY, FITNESS
 * FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE AUTHORS OR
 * COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER LIABILITY, WHETHER
 * IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM, OUT OF OR IN
 * CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE SOFTWARE.
 *
 */

#include <string.h>
#include "sys/queue.h"
#include "audio_error.h"
#include "audio_mem.h"
#include "dram_list.h"

static const char *TAG = "DRAM_LIST";

/**
 * @brief List node save URL
 */
typedef struct url_info {
    char *url_name;                /*!< URL string */
    TAILQ_ENTRY(url_info) entries; /*!< list node */
} url_info_t;

/**
 * @brief Dram list management unit
 */
typedef struct dram_list {
    uint16_t url_num;                         /*!< Number of URLs in dram playlist */
    url_info_t *cur_node;                     /*!< Current node of URL on dram list */
    TAILQ_HEAD(info, url_info) url_info_list; /*!< List of URL nodes */
} dram_list_t;

esp_err_t dram_list_get_operation(playlist_operation_t *operation);

esp_err_t dram_list_create(playlist_operator_handle_t *handle)
{
    AUDIO_NULL_CHECK(TAG, handle, return ESP_FAIL);
    playlist_operator_handle_t dram_handle = (playlist_operator_handle_t )audio_calloc(1, sizeof(playlist_operator_t));
    AUDIO_NULL_CHECK(TAG, dram_handle, return ESP_FAIL);

    dram_list_t *dram_list = (dram_list_t *) audio_calloc(1, sizeof(dram_list_t));
    AUDIO_NULL_CHECK(TAG, dram_list, {
        free(dram_handle);
        return ESP_FAIL;
    });

    dram_handle->playlist = dram_list;
    dram_handle->get_operation = dram_list_get_operation;

    TAILQ_INIT(&dram_list->url_info_list);
    *handle = dram_handle;
    return ESP_OK;
}

esp_err_t dram_list_save(playlist_operator_handle_t handle, const char *url)
{
    AUDIO_NULL_CHECK(TAG, handle, return ESP_FAIL);
    AUDIO_NULL_CHECK(TAG, url, return ESP_FAIL);
    dram_list_t *playlist = handle->playlist;
    AUDIO_NULL_CHECK(TAG, playlist, return ESP_FAIL);

    url_info_t *list_node = (url_info_t *)audio_calloc(1, sizeof(url_info_t));
    AUDIO_NULL_CHECK(TAG, list_node, return ESP_FAIL);
    list_node->url_name = (char *)audio_calloc(1, strlen(url) + 1);
    AUDIO_NULL_CHECK(TAG, list_node->url_name, {
        free(list_node);
        list_node = NULL;
        return ESP_FAIL;
    });

    strncpy(list_node->url_name, url, strlen(url));
    TAILQ_INSERT_TAIL(&playlist->url_info_list, list_node, entries);

    if (NULL == playlist->cur_node) {
        ESP_LOGD(TAG, "Set the first url as the default url");
        playlist->cur_node = list_node;
    }
    playlist->url_num ++;
    return ESP_OK;
}

esp_err_t dram_list_next(playlist_operator_handle_t handle, int step, char **url_buff)
{
    AUDIO_NULL_CHECK(TAG, handle, return ESP_FAIL);
    AUDIO_NULL_CHECK(TAG, url_buff, return ESP_FAIL);
    dram_list_t *playlist = handle->playlist;
    AUDIO_NULL_CHECK(TAG, playlist, return ESP_FAIL);

    if (playlist->url_num == 0) {
        ESP_LOGE(TAG, "Please add urls to playlist first");
        return ESP_FAIL;
    }
    if (step < 0) {
        ESP_LOGE(TAG, "Number of steps should be larger than 0");
        return ESP_FAIL;
    }

    url_info_t *tmp = playlist->cur_node;
    for (int i = 0; i < step; i++) {
        tmp = TAILQ_NEXT(playlist->cur_node, entries);
        if (tmp == NULL) {
            ESP_LOGD(TAG, "Already the last url of the playlist");
            tmp = TAILQ_FIRST(&playlist->url_info_list);
        }
        playlist->cur_node = tmp;
    }
    *url_buff = tmp->url_name;

    return ESP_OK;
}

esp_err_t dram_list_prev(playlist_operator_handle_t handle, int step, char **url_buff)
{
    AUDIO_NULL_CHECK(TAG, handle, return ESP_FAIL);
    AUDIO_NULL_CHECK(TAG, url_buff, return ESP_FAIL);
    dram_list_t *playlist = handle->playlist;
    AUDIO_NULL_CHECK(TAG, playlist, return ESP_FAIL);

    if (playlist->url_num == 0) {
        ESP_LOGE(TAG, "Please add urls to playlist first");
        return ESP_FAIL;
    }
    if (step < 0) {
        ESP_LOGE(TAG, "Number of steps should be larger than 0");
        return ESP_FAIL;
    }

    url_info_t *tmp = playlist->cur_node;
    for (int i = 0; i < step; i++) {
        tmp = TAILQ_PREV(playlist->cur_node, info, entries);
        if (tmp == NULL) {
            ESP_LOGD(TAG, "Already the first url of the playlist");
            tmp = TAILQ_LAST(&playlist->url_info_list, info);
        }
        playlist->cur_node = tmp;
    }
    *url_buff = tmp->url_name;

    return ESP_OK;
}

esp_err_t dram_list_current(playlist_operator_handle_t handle, char **url_buff)
{
    AUDIO_NULL_CHECK(TAG, handle, return ESP_FAIL);
    AUDIO_NULL_CHECK(TAG, url_buff, return ESP_FAIL);
    dram_list_t *playlist = handle->playlist;
    AUDIO_NULL_CHECK(TAG, playlist, return ESP_FAIL);

    if (playlist->url_num == 0) {
        ESP_LOGE(TAG, "Please add urls to playlist first");
        return ESP_FAIL;
    }

    url_info_t *cur_node = playlist->cur_node;
    *url_buff = cur_node->url_name;
    return ESP_OK;
}

esp_err_t dram_list_show(playlist_operator_handle_t handle)
{
    AUDIO_NULL_CHECK(TAG, handle, return ESP_FAIL);
    dram_list_t *playlist = handle->playlist;
    AUDIO_NULL_CHECK(TAG, playlist, return ESP_FAIL);

    url_info_t *item, *tmp;
    TAILQ_FOREACH_SAFE(item, &playlist->url_info_list, entries, tmp) {
        ESP_LOGI(TAG, "URL: %s", item->url_name);
    }
    return ESP_OK;
}

int dram_list_get_url_num(playlist_operator_handle_t handle)
{
    AUDIO_NULL_CHECK(TAG, handle, return ESP_FAIL);
    dram_list_t *playlist = handle->playlist;
    AUDIO_NULL_CHECK(TAG, playlist, return ESP_FAIL);

    return playlist->url_num;
}

esp_err_t dram_list_destroy(playlist_operator_handle_t handle)
{
    AUDIO_NULL_CHECK(TAG, handle, return ESP_FAIL);
    dram_list_t *playlist = handle->playlist;
    AUDIO_NULL_CHECK(TAG, playlist, return ESP_FAIL);

    url_info_t *item, *tmp;
    TAILQ_FOREACH_SAFE(item, &playlist->url_info_list, entries, tmp) {
        free(item->url_name);
        item->url_name = NULL;
        TAILQ_REMOVE(&playlist->url_info_list, item, entries);
        free(item);
        item = NULL;
    }
    free(playlist);
    handle->playlist = NULL;
    free(handle);
    return ESP_OK;
}

esp_err_t dram_list_get_operation(playlist_operation_t *operation)
{
    AUDIO_NULL_CHECK(TAG, operation, return ESP_FAIL);
    operation->show = (void *)dram_list_show;
    operation->save = (void *)dram_list_save;
    operation->next = (void *)dram_list_next;
    operation->prev = (void *)dram_list_prev;
    operation->current = (void *)dram_list_current;
    operation->destroy = (void *)dram_list_destroy;
    operation->get_url_num = (void *)dram_list_get_url_num;
    operation->type = PLAYLIST_DRAM;
    return ESP_OK;
}
