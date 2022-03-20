// Copyright 2008 Dolphin Emulator Project
// SPDX-License-Identifier: GPL-2.0-or-later

#include "VideoCommon/IndexGenerator.h"

#include <array>
#include <cstring>

#include "Common/CommonTypes.h"
#include "Common/Logging/Log.h"
#include "VideoCommon/OpcodeDecoding.h"
#include "VideoCommon/VideoConfig.h"

namespace
{
// Triangles
u16* AddList(u16* index_ptr, u32 num_verts, u32 index)
{
  bool ccw = bpmem.genMode.cullmode == CullMode::Front;
  int v1 = ccw ? 0 : 1;
  int v2 = ccw ? 1 : 0;
  for (u32 i = 2; i < num_verts; i += 3)
  {
    *index_ptr++ = index + i - 2;
    *index_ptr++ = index + i - v1;
    *index_ptr++ = index + i - v2;
  }
  return index_ptr;
}

u16* AddStrip(u16* index_ptr, u32 num_verts, u32 index)
{
  bool ccw = bpmem.genMode.cullmode == CullMode::Front;
  int wind = ccw ? 0 : 1;
  for (u32 i = 2; i < num_verts; ++i)
  {
    *index_ptr++ = index + i - 2;
    *index_ptr++ = index + i - wind;
    wind ^= 1;  // toggle between 0 and 1
    *index_ptr++ = index + i - wind;
  }
  return index_ptr;
}

/**
 * FAN simulator:
 *
 *   2---3
 *  / \ / \
 * 1---0---4
 *
 * would generate this triangles:
 * 012, 023, 034
 *
 * rotated (for better striping):
 * 120, 302, 034
 *
 * as odd ones have to winded, following strip is fine:
 * 12034
 *
 * so we use 6 indices for 3 triangles
 */

u16* AddFan(u16* index_ptr, u32 num_verts, u32 index)
{
  bool ccw = bpmem.genMode.cullmode == CullMode::Front;
  int v1 = ccw ? 0 : 1;
  int v2 = ccw ? 1 : 0;
  for (u32 i = 2; i < num_verts; ++i)
  {
    *index_ptr++ = index;
    *index_ptr++ = index + i - v1;
    *index_ptr++ = index + i - v2;
  }
  return index_ptr;
}

/**
 * QUAD simulator
 *
 * 0---1   4---5
 * |\  |   |\  |
 * | \ |   | \ |
 * |  \|   |  \|
 * 3---2   7---6
 *
 * 012,023, 456,467 ...
 * or 120,302, 564,746
 * or as strip: 1203, 5647
 *
 * Warning:
 * A simple triangle has to be rendered for three vertices.
 * ZWW do this for sun rays
 */
u16* AddQuads(u16* index_ptr, u32 num_verts, u32 index)
{
  bool ccw = bpmem.genMode.cullmode == CullMode::Front;
  u32 i = 3;
  int v1 = ccw ? 1 : 2;
  int v2 = ccw ? 2 : 1;
  int v3 = ccw ? 0 : 1;
  int v4 = ccw ? 1 : 0;

  for (; i < num_verts; i += 4)
  {
    *index_ptr++ = index + i - 3;
    *index_ptr++ = index + i - v1;
    *index_ptr++ = index + i - v2;

    *index_ptr++ = index + i - 3;
    *index_ptr++ = index + i - v3;
    *index_ptr++ = index + i - v4;
  }

  // Legend of Zelda The Wind Waker
  // if three vertices remaining, render a triangle
  if (i == num_verts)
  {
    *index_ptr++ = index + i - 3;
    *index_ptr++ = index + i - v1;
    *index_ptr++ = index + i - v2;
  }

  return index_ptr;
}

u16* AddQuads_nonstandard(u16* index_ptr, u32 num_verts, u32 index)
{
  WARN_LOG_FMT(VIDEO, "Non-standard primitive drawing command GL_DRAW_QUADS_2");
  return AddQuads(index_ptr, num_verts, index);
}

u16* AddLineList(u16* index_ptr, u32 num_verts, u32 index)
{
  for (u32 i = 1; i < num_verts; i += 2)
  {
    *index_ptr++ = index + i - 1;
    *index_ptr++ = index + i;
  }
  return index_ptr;
}

// Shouldn't be used as strips as LineLists are much more common
// so converting them to lists
u16* AddLineStrip(u16* index_ptr, u32 num_verts, u32 index)
{
  for (u32 i = 0; i < num_verts; ++i)
  {
    *index_ptr++ = index + i;
    *index_ptr++ = index + i + 1;
  }
  return index_ptr;
}

u16* AddPoints(u16* index_ptr, u32 num_verts, u32 index)
{
  for (u32 i = 0; i != num_verts; ++i)
  {
    *index_ptr++ = index + i;
  }
  return index_ptr;
}
}  // Anonymous namespace

void IndexGenerator::Start(u16* index_ptr)
{
  m_index_buffer_current = index_ptr;
  m_base_index_ptr = index_ptr;
  m_base_index = 0;
}

void IndexGenerator::AddIndices(int primitive, u32 num_vertices)
{
  switch (primitive)
  {
    case OpcodeDecoder::GX_DRAW_QUADS:
      m_index_buffer_current = AddQuads(m_index_buffer_current, num_vertices, m_base_index);
          break;
    case OpcodeDecoder::GX_DRAW_QUADS_2:
      m_index_buffer_current = AddQuads_nonstandard(m_index_buffer_current, num_vertices, m_base_index);
          break;
    case OpcodeDecoder::GX_DRAW_TRIANGLES:
      m_index_buffer_current = AddList(m_index_buffer_current, num_vertices, m_base_index);
          break;
    case OpcodeDecoder::GX_DRAW_TRIANGLE_STRIP:
      m_index_buffer_current = AddStrip(m_index_buffer_current, num_vertices, m_base_index);
          break;
    case OpcodeDecoder::GX_DRAW_TRIANGLE_FAN:
      m_index_buffer_current = AddFan(m_index_buffer_current, num_vertices, m_base_index);
          break;
    case OpcodeDecoder::GX_DRAW_LINES:
      m_index_buffer_current = AddLineList(m_index_buffer_current, num_vertices, m_base_index);
          break;
    case OpcodeDecoder::GX_DRAW_LINE_STRIP:
      m_index_buffer_current = AddLineStrip(m_index_buffer_current, num_vertices, m_base_index);
          break;
    case OpcodeDecoder::GX_DRAW_POINTS:
      m_index_buffer_current = AddPoints(m_index_buffer_current, num_vertices, m_base_index);
          break;
  }
  m_base_index += num_vertices;
}

void IndexGenerator::AddExternalIndices(const u16* indices, u32 num_indices, u32 num_vertices)
{
  std::memcpy(m_index_buffer_current, indices, sizeof(u16) * num_indices);
  m_index_buffer_current += num_indices;
  m_base_index += num_vertices;
}