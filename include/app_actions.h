#pragma once

#include <string>
#include <vector>

#ifdef __linux__
#include <gtk/gtk.h>
#endif

class Goose;

#ifdef __linux__
void AppActions_SetApplication(GtkApplication* app);
#else
void AppActions_SetApplication(void* app);
#endif

void AppActions_EnsureInitialGoose();
Goose* AppActions_SpawnGoose(const std::string& name = "");
void AppActions_ClearGeese();
void AppActions_Quit();
std::string AppActions_GetStatus();
std::string AppActions_HandleCommand(const std::vector<std::string>& args);
