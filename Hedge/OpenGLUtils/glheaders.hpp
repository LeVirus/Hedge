#pragma once

#ifdef _WIN32
#ifndef NOMINMAX
#define NOMINMAX
#endif
#ifndef WIN32_LEAN_AND_MEAN
#define WIN32_LEAN_AND_MEAN
#endif
#endif
#define APIENTRY        // vide → supprime __stdcall
#define GLAD_API_PTR    // vide ou juste *
#define WINAPI

#include <glad/glad.h>
#include <GLFW/glfw3.h>
