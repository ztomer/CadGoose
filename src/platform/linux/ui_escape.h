#ifndef UI_ESCAPE_H
#define UI_ESCAPE_H

#include <gtk/gtk.h>

void AttachEmergencyEscController(GtkWidget* window);
void ResetEscapeHoldState();
void ClearAllGooseState();
void UpdateEscapeHoldHud();
void MaybeTriggerEscapeKill();

#endif