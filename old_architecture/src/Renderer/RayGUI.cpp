/*
** EPITECH PROJECT, 2026
** Prototype
** File description:
** RayGUI implementation unit
*/

#ifndef RAYLIB_NO_WINDOWS_H
#define RAYLIB_NO_WINDOWS_H
#endif

#define RAYGUI_IMPLEMENTATION

#if defined(__GNUC__)
#pragma GCC diagnostic push
#pragma GCC diagnostic ignored "-Wconversion"
#pragma GCC diagnostic ignored "-Wsign-conversion"
#pragma GCC diagnostic ignored "-Wmissing-field-initializers"
#pragma GCC diagnostic ignored "-Wunused-parameter"
#pragma GCC diagnostic ignored "-Wshadow"
#pragma GCC diagnostic ignored "-Wmissing-declarations"
#pragma GCC diagnostic ignored "-Wcast-qual"
#pragma GCC diagnostic ignored "-Wunused-but-set-variable"
#pragma GCC diagnostic ignored "-Wcast-align"
#if !defined(__clang__)
#pragma GCC diagnostic ignored "-Wstringop-overflow"
#endif
#endif

#include <Renderer/RayGUI.hpp>
#include <map>
#include <string>
#include <utility>

#if defined(__GNUC__)
#pragma GCC diagnostic pop
#endif
