#include "StateManager.h"

// ===== Global Variables =====
StateManager& state = StateManager::instance();

void setup() {
  // Initialize Serial for debugging
  while (!Serial && millis() < 3000); // Wait up to 3 seconds for Serial: this is for computer connection

  // Initialize StateManager (this initializes all subsystems)
  state.init();
}

void loop() {
  unsigned long now = millis();
  
  // Update all subsystems through StateManager
  state.update(now);
  
  // Optional: Add a small delay to prevent overwhelming the system
  // Remove this if you need maximum responsiveness
  delay(10);
}