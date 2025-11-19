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
*/

#pragma once

#include "moss/engine.h"
#include "moss/result.h"
#include "moss/sprite.h"

/*=============================================================================
    STRUCTURES
  =============================================================================*/

typedef struct MossSpriteBatch MossSpriteBatch;

typedef struct
{
  MossEngine *engine;   /* Engine handle. */
  size_t      capacity; /* Maximum number of sprites that can be added to this batch. */
} MossSpriteBatchCreateInfo;

typedef struct
{
  const MossSprite *sprites;
  size_t            sprite_count;
} MossAddSpritesToSpriteBatchInfo;

/*=============================================================================
    FUNCTIONS
  =============================================================================*/

MossResult
moss_create_sprite_batch (const MossSpriteBatchCreateInfo *info, MossSpriteBatch **out_sprite_batch);

void moss_destroy_sprite_batch (MossSpriteBatch *sprite_batch);

void moss_clear_sprite_batch (MossSpriteBatch *sprite_batch);

MossResult moss_begin_sprite_batch (MossSpriteBatch *sprite_batch);

MossResult moss_add_sprites_to_sprite_batch (
  MossSpriteBatch                       *sprite_batch,
  const MossAddSpritesToSpriteBatchInfo *info
);

MossResult moss_end_sprite_batch (MossSpriteBatch *sprite_batch);

MossResult moss_draw_sprite_batch (MossEngine *engine, MossSpriteBatch *sprite_batch);
