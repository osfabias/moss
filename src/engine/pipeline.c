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

#include <assert.h>
#include <stdbool.h>
#include <stdint.h>

#include <vulkan/vulkan.h>

#include "moss/result.h"

#include "src/internal/engine.h"
#include "src/internal/log.h"
#include "src/internal/shaders.h"
#include "src/internal/vertex.h"
#include "src/internal/vulkan/utils/shader.h"

#include "src/engine/engine_internal.h"

/*=============================================================================
    INTERNAL FUNCTIONS IMPLEMENTATION
  =============================================================================*/

void moss__createVkPipelineVertexInputStateInfo (
  VkPipelineVertexInputStateCreateInfo *const pOutInfo
)
{
  const Moss__VkVertexInputBindingDescriptionPack bindingDescriptionsPack =
    moss__getVkVertexInputBindingDescription ( );

  const Moss__VkVertexInputAttributeDescriptionPack attributeDescriptionsPack =
    moss__getVkVertexInputAttributeDescription ( );

  *pOutInfo = (VkPipelineVertexInputStateCreateInfo) {
    .sType = VK_STRUCTURE_TYPE_PIPELINE_VERTEX_INPUT_STATE_CREATE_INFO,
    .vertexBindingDescriptionCount   = bindingDescriptionsPack.count,
    .pVertexBindingDescriptions      = bindingDescriptionsPack.descriptions,
    .vertexAttributeDescriptionCount = attributeDescriptionsPack.count,
    .pVertexAttributeDescriptions    = attributeDescriptionsPack.descriptions,
  };
}

MossResult moss__createGraphicsPipeline (MossEngine *const pEngine)
{
  VkShaderModule vertShaderModule;
  VkShaderModule fragShaderModule;

  {
    const MossVk__CreateShaderModuleFromFileInfo createInfo = {
      .device          = pEngine->device,
      .filePath        = MOSS__VERT_SHADER_PATH,
      .outShaderModule = &vertShaderModule,
    };
    const MossResult result = mossVk__createShaderModuleFromFile (&createInfo);
    if (result != MOSS_RESULT_SUCCESS)
    {
      moss__error ("Failed to create fragment shader module.");
      return MOSS_RESULT_ERROR;
    }
  }

  {
    const MossVk__CreateShaderModuleFromFileInfo createInfo = {
      .device          = pEngine->device,
      .filePath        = MOSS__FRAG_SHADER_PATH,
      .outShaderModule = &fragShaderModule,
    };
    const MossResult result = mossVk__createShaderModuleFromFile (&createInfo);
    if (result != MOSS_RESULT_SUCCESS)
    {
      vkDestroyShaderModule (pEngine->device, vertShaderModule, NULL);

      moss__error ("Failed to create fragment shader module.\n");
      return MOSS_RESULT_ERROR;
    }
  }

  const VkPipelineShaderStageCreateInfo vert_shader_stage_info = {
    .sType  = VK_STRUCTURE_TYPE_PIPELINE_SHADER_STAGE_CREATE_INFO,
    .stage  = VK_SHADER_STAGE_VERTEX_BIT,
    .module = vertShaderModule,
    .pName  = "main",
  };

  const VkPipelineShaderStageCreateInfo frag_shader_stage_info = {
    .sType  = VK_STRUCTURE_TYPE_PIPELINE_SHADER_STAGE_CREATE_INFO,
    .stage  = VK_SHADER_STAGE_FRAGMENT_BIT,
    .module = fragShaderModule,
    .pName  = "main",
  };

  const VkPipelineShaderStageCreateInfo shader_stages[] = { vert_shader_stage_info,
                                                            frag_shader_stage_info };

  VkPipelineVertexInputStateCreateInfo vertex_input_info;
  moss__createVkPipelineVertexInputStateInfo (&vertex_input_info);

  const VkPipelineInputAssemblyStateCreateInfo input_assembly = {
    .sType                  = VK_STRUCTURE_TYPE_PIPELINE_INPUT_ASSEMBLY_STATE_CREATE_INFO,
    .topology               = VK_PRIMITIVE_TOPOLOGY_TRIANGLE_LIST,
    .primitiveRestartEnable = VK_FALSE,
  };

  const VkPipelineViewportStateCreateInfo viewport_state = {
    .sType         = VK_STRUCTURE_TYPE_PIPELINE_VIEWPORT_STATE_CREATE_INFO,
    .viewportCount = 1,
    .pViewports    = NULL,  // Dynamic viewport
    .scissorCount  = 1,
    .pScissors     = NULL,  // Dynamic scissor
  };

  const VkPipelineRasterizationStateCreateInfo rasterizer = {
    .sType                   = VK_STRUCTURE_TYPE_PIPELINE_RASTERIZATION_STATE_CREATE_INFO,
    .depthClampEnable        = VK_FALSE,
    .rasterizerDiscardEnable = VK_FALSE,
    .polygonMode             = VK_POLYGON_MODE_FILL,
    .lineWidth               = 1.0F,
    .cullMode                = VK_CULL_MODE_NONE,
    .frontFace               = VK_FRONT_FACE_COUNTER_CLOCKWISE,
    .depthBiasEnable         = VK_FALSE,
  };

  const VkPipelineMultisampleStateCreateInfo multisampling = {
    .sType                = VK_STRUCTURE_TYPE_PIPELINE_MULTISAMPLE_STATE_CREATE_INFO,
    .sampleShadingEnable  = VK_FALSE,
    .rasterizationSamples = VK_SAMPLE_COUNT_1_BIT,
  };

  const VkPipelineColorBlendAttachmentState color_blend_attachment = {
    .colorWriteMask = VK_COLOR_COMPONENT_R_BIT | VK_COLOR_COMPONENT_G_BIT |
                      VK_COLOR_COMPONENT_B_BIT | VK_COLOR_COMPONENT_A_BIT,
    .blendEnable         = VK_TRUE,
    .srcColorBlendFactor = VK_BLEND_FACTOR_SRC_ALPHA,
    .dstColorBlendFactor = VK_BLEND_FACTOR_ONE_MINUS_SRC_ALPHA,
    .colorBlendOp        = VK_BLEND_OP_ADD,
    .srcAlphaBlendFactor = VK_BLEND_FACTOR_ONE,
    .dstAlphaBlendFactor = VK_BLEND_FACTOR_ONE_MINUS_SRC_ALPHA,
    .alphaBlendOp        = VK_BLEND_OP_ADD,
  };

  const VkPipelineColorBlendStateCreateInfo color_blending = {
    .sType           = VK_STRUCTURE_TYPE_PIPELINE_COLOR_BLEND_STATE_CREATE_INFO,
    .logicOpEnable   = VK_FALSE,
    .attachmentCount = 1,
    .pAttachments    = &color_blend_attachment,
  };

  const VkDescriptorSetLayout set_layouts[] = { pEngine->descriptorSetLayout };

  const VkPipelineLayoutCreateInfo pipeline_layout_info = {
    .sType                  = VK_STRUCTURE_TYPE_PIPELINE_LAYOUT_CREATE_INFO,
    .pNext                  = NULL,
    .setLayoutCount         = sizeof (set_layouts) / sizeof (set_layouts[ 0 ]),
    .pSetLayouts            = set_layouts,
    .pushConstantRangeCount = 0,
    .pPushConstantRanges    = NULL,
  };

  if (vkCreatePipelineLayout (
        pEngine->device,
        &pipeline_layout_info,
        NULL,
        &pEngine->pipelineLayout
      ) != VK_SUCCESS)
  {
    moss__error ("Failed to create pipeline layout.\n");
    vkDestroyShaderModule (pEngine->device, fragShaderModule, NULL);
    vkDestroyShaderModule (pEngine->device, vertShaderModule, NULL);
    return MOSS_RESULT_ERROR;
  }

  const VkDynamicState dynamic_states[] = {
    VK_DYNAMIC_STATE_VIEWPORT,
    VK_DYNAMIC_STATE_SCISSOR,
  };

  const VkPipelineDynamicStateCreateInfo dynamic_state = {
    .sType             = VK_STRUCTURE_TYPE_PIPELINE_DYNAMIC_STATE_CREATE_INFO,
    .dynamicStateCount = sizeof (dynamic_states) / sizeof (dynamic_states[ 0 ]),
    .pDynamicStates    = dynamic_states,
  };

  const VkPipelineDepthStencilStateCreateInfo depth_stencil_state = {
    .sType                 = VK_STRUCTURE_TYPE_PIPELINE_DEPTH_STENCIL_STATE_CREATE_INFO,
    .pNext                 = VK_FALSE,
    .depthTestEnable       = VK_TRUE,
    .depthWriteEnable      = VK_TRUE,
    .depthCompareOp        = VK_COMPARE_OP_LESS,
    .depthBoundsTestEnable = VK_FALSE,
    .minDepthBounds        = 0.0F,
    .maxDepthBounds        = 0.0F,
    .stencilTestEnable     = VK_FALSE,
    .front                 = { 0 },
    .back                  = { 0 },
  };

  const VkGraphicsPipelineCreateInfo pipeline_info = {
    .sType               = VK_STRUCTURE_TYPE_GRAPHICS_PIPELINE_CREATE_INFO,
    .pNext               = NULL,
    .stageCount          = 2,
    .pStages             = shader_stages,
    .pVertexInputState   = &vertex_input_info,
    .pInputAssemblyState = &input_assembly,
    .pViewportState      = &viewport_state,
    .pRasterizationState = &rasterizer,
    .pMultisampleState   = &multisampling,
    .pColorBlendState    = &color_blending,
    .pDynamicState       = &dynamic_state,
    .pDepthStencilState  = &depth_stencil_state,
    .layout              = pEngine->pipelineLayout,
    .renderPass          = pEngine->renderPass,
    .subpass             = 0,
    .basePipelineHandle  = VK_NULL_HANDLE,
    .basePipelineIndex   = -1,
  };

  const VkResult result = vkCreateGraphicsPipelines (
    pEngine->device,
    VK_NULL_HANDLE,
    1,
    &pipeline_info,
    NULL,
    &pEngine->graphicsPipeline
  );

  vkDestroyShaderModule (pEngine->device, fragShaderModule, NULL);
  vkDestroyShaderModule (pEngine->device, vertShaderModule, NULL);

  if (result != VK_SUCCESS)
  {
    moss__error ("Failed to create graphics pipeline. Error code: %d.\n", result);
    vkDestroyPipelineLayout (pEngine->device, pEngine->pipelineLayout, NULL);
    return MOSS_RESULT_ERROR;
  }

  return MOSS_RESULT_SUCCESS;
}

