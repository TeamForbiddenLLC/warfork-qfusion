/*
* Copyright (C) 1997-2001 Id Software, Inc.
* 
* This program is free software; you can redistribute it and/or
* modify it under the terms of the GNU General Public License
* as published by the Free Software Foundation; either version 2
* of the License, or (at your option) any later version.
* 
* This program is distributed in the hope that it will be useful,
* but WITHOUT ANY WARRANTY; without even the implied warranty of
* MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.
* 
* See the GNU General Public License for more details.
* 
* You should have received a copy of the GNU General Public License
* along with this program; if not, write to the Free Software
* Foundation, Inc., 59 Temple Place - Suite 330, Boston, MA  02111-1307, USA.
* 
*/

/*
 * Copyright (c) 2017-2024 The Forge Interactive Inc.
 *
 * This file is part of The-Forge
 * (see https://github.com/ConfettiFX/The-Forge).
 *
 * Licensed to the Apache Software Foundation (ASF) under one
 * or more contributor license agreements.  See the NOTICE file
 * distributed with this work for additional information
 * regarding copyright ownership.  The ASF licenses this file
 * to you under the Apache License, Version 2.0 (the
 * "License"); you may not use this file except in compliance
 * with the License.  You may obtain a copy of the License at
 *
 *   http://www.apache.org/licenses/LICENSE-2.0
 *
 * Unless required by applicable law or agreed to in writing,
 * software distributed under the License is distributed on an
 * "AS IS" BASIS, WITHOUT WARRANTIES OR CONDITIONS OF ANY
 * KIND, either express or implied.  See the License for the
 * specific language governing permissions and limitations
 * under the License.
 */

#ifndef _Q_TYPES_H
#define _Q_TYPES_H

#include <stdint.h>

#if INTPTR_MAX == 0x7FFFFFFFFFFFFFFFLL
  #define Q_PTR_SIZE 8
#elif INTPTR_MAX == 0x7FFFFFFF
  #define Q_PTR_SIZE 4
#else
  #error unsupported platform
#endif

#define Q_ALIGN_TO(size, alignment) (((size) + (alignment)-1) & ~((alignment)-1))
#define Q_ARRAY_COUNT(array) (sizeof(array) / (sizeof(array[0]) * (sizeof(array) != Q_PTR_SIZE || sizeof(array[0]) <= Q_PTR_SIZE)))
#define Q_MIN(a, b) ((a) < (b) ? (a) : (b))
#define Q_MAX(a, b) ((a) > (b) ? (a) : (b))

#ifdef __cplusplus
#define Q_CONSTEXPR constexpr
#define Q_EXTERN_C  extern "C"
#else
#define Q_CONSTEXPR
#define Q_EXTERN_C
#endif

#ifdef __cplusplus
#define Q_ENUM_FLAG(TYPE, ENUM_TYPE)                                                                                      \
    inline Q_CONSTEXPR ENUM_TYPE operator|(ENUM_TYPE a, ENUM_TYPE b) { return ENUM_TYPE(((TYPE)a) | ((TYPE)b)); }        \
    inline ENUM_TYPE&                operator|=(ENUM_TYPE& a, ENUM_TYPE b) { return (ENUM_TYPE&)(((TYPE&)a) |= ((TYPE)b)); } \
    inline Q_CONSTEXPR ENUM_TYPE operator&(ENUM_TYPE a, ENUM_TYPE b) { return ENUM_TYPE(((TYPE)a) & ((TYPE)b)); }        \
    inline ENUM_TYPE&                operator&=(ENUM_TYPE& a, ENUM_TYPE b) { return (ENUM_TYPE&)(((TYPE&)a) &= ((TYPE)b)); } \
    inline Q_CONSTEXPR ENUM_TYPE operator~(ENUM_TYPE a) { return ENUM_TYPE(~((TYPE)a)); }                                \
    inline Q_CONSTEXPR ENUM_TYPE operator^(ENUM_TYPE a, ENUM_TYPE b) { return ENUM_TYPE(((TYPE)a) ^ ((TYPE)b)); }        \
    inline ENUM_TYPE&                operator^=(ENUM_TYPE& a, ENUM_TYPE b) { return (ENUM_TYPE&)(((TYPE&)a) ^= ((TYPE)b)); }
#else
#define Q_ENUM_FLAG(TYPE, ENUM_TYPE)
#endif

#if defined(__clang__) && !defined(__cplusplus)
#define Q_COMPILE_ASSERT(exp) _Static_assert(exp, #exp)
#else
#define Q_COMPILE_ASSERT(exp) static_assert(exp, #exp)
#endif

#if defined(_MSC_VER)
#define Q_EXPORT __declspec(dllexport)
#define Q_IMPORT __declspec(dllimport)
#elif defined(__GNUC__) // clang & gcc
#define Q_EXPORT __attribute__((visibility("default")))
#define Q_IMPORT
#endif

#endif

