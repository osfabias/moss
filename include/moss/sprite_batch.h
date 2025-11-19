/*
  Copyright 2025 Osfabias

  Licensed under the Apache License, Version 2.0 (the "License");
  you may not use this file except in compliance with the License.
  You may obtain a copy of the License at

      http://www.apache.org/licenses/LICENSE-2.0

  Unless required by applicable law or agreed to in writing, software
  distributed under the License is distributed on an "AS IS" BASIS,
  WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
  See the License for the specific language governing permissions and
  limitations under the License.

  @file include/moss/sprite_batch.c
  @brief Sprite batch struct and function declarations.
  @author Ilya Buravov (ilburale@gmail.com)
*/

#pragma once

#include "moss/engine.h"
#include "moss/result.h"
#include "moss/sprite.h"

/*=============================================================================
    STRUCTURES
  =============================================================================*/

/*
  @brief Sprite batch.
*/
typedef struct MossSpriteBatch MossSpriteBatch;

/*
  @brief Sprite batch create info.
*/
typedef struct
{
  MossEngine *engine;   /* Engine handle. */
  size_t      capacity; /* Maximum number of sprites that can be added to this batch. */
} MossSpriteBatchCreateInfo;

/*
  @brief Sprites addition operation info.
*/
typedef struct
{
  const MossSprite *sprites;
  size_t            sprite_count;
} MossAddSpritesToSpriteBatchInfo;

/*=============================================================================
    FUNCTIONS
  =============================================================================*/

/*
  @brief Creates sprite batch.
  @param info Required info to create sprite batch.
  @param out_sprite_batch Output parameter for created sprite batch.
  @return MOSS_RESULT_SUCCESS on success, MOSS_RESULT_ERROR otherwise.
*/
MossResult
moss_create_sprite_batch (const MossSpriteBatchCreateInfo *info, MossSpriteBatch **out_sprite_batch);

/*
  @brief Destroys sprite batch.
  @param sprite_batch Sprite batch handle.
*/
void moss_destroy_sprite_batch (MossSpriteBatch *sprite_batch);

/*
  @brief Clears sprite batch from previosly added sprites.
  @param sprite_batch Sprite batch handle.
*/
void moss_clear_sprite_batch (MossSpriteBatch *sprite_batch);

/*
  @brief Begins new sprite batch.
  @param sprite_batch Sprite batch handle.
  @return MOSS_RESULT_SUCCESS on success, otherwise returns error code.
  @note It's required to run this function before adding new sprites to the batch.
  @note Previously added sprites won't be removed from the batch.
*/
MossResult moss_begin_sprite_batch (MossSpriteBatch *sprite_batch);

/*
  @brief Adds sprite to the sprite batch.
  @param sprite_batch Sprite batch handle.
  @param info Required operation info.
  @return Returns MOSS_RESULT_SUCCESS on success, otherwise returns error code.
  @warning Make sure that you beginned passed sprite batch before calling this function.
*/
MossResult moss_add_sprites_to_sprite_batch (
  MossSpriteBatch                       *sprite_batch,
  const MossAddSpritesToSpriteBatchInfo *info
);

/*
  @brief End sprite batch.
  @param sprite_batch Sprite batch handle.
  @return Returns MOSS_RESULT_SUCCESS on success, otherwise returns error code.
  @note It's required to end sprite batch before attempting to draw it.
*/
MossResult moss_end_sprite_batch (MossSpriteBatch *sprite_batch);

/*
  @brief Draws sprite batch.
  @param engine Engine handle.
  @param sprite_batch Sprite batch handle.
  @return Returns MOSS_RESULT_SUCCESS on success, otherwise returns error code.
  @warning Make sure that you ended passed sprite batch before calling this function.
*/
MossResult moss_draw_sprite_batch (MossEngine *engine, MossSpriteBatch *sprite_batch);
