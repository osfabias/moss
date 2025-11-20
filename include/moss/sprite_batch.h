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
  MossEngine *pEngine;  /* Engine handle. */
  size_t      capacity; /* Maximum number of sprites that can be added to this batch. */
} MossSpriteBatchCreateInfo;

typedef struct
{
  const MossSprite *pSprites;
  size_t            spriteCount;
} MossAddSpritesToSpriteBatchInfo;

/*=============================================================================
    FUNCTIONS
  =============================================================================*/

MossResult mossCreateSpriteBatch (
  const MossSpriteBatchCreateInfo *pInfo,
  MossSpriteBatch                **pOutSpriteBatch
);

void mossDestroySpriteBatch (MossSpriteBatch *pSpriteBatch);

void mossClearSpriteBatch (MossSpriteBatch *pSpriteBatch);

MossResult mossBeginSpriteBatch (MossSpriteBatch *pSpriteBatch);

MossResult mossAddSpritesToSpriteBatch (
  MossSpriteBatch                       *pSpriteBatch,
  const MossAddSpritesToSpriteBatchInfo *pInfo
);

MossResult mossEndSpriteBatch (MossSpriteBatch *pSpriteBatch);

MossResult mossDrawSpriteBatch (MossEngine *pEngine, MossSpriteBatch *pSpriteBatch);
