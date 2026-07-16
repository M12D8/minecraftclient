# Vape v4 Style Guide

This guide explains how to implement Vape v4-style features in the Fabric mod.

## Vape v4 Characteristics

### Visual Style
- **Dark Theme**: Almost black background (#121212)
- **Minimal Design**: Clean, simple UI
- **Blue Accent**: Vape blue (#3E80FF) for highlights
- **Compact Layout**: Features listed vertically
- **Professional**: Looks like enterprise software

### HUD Layout
```
┌─────────────────────┐
│ COMBAT          [×] │  ← Category header with close button
├─────────────────────┤
│■ Kill Aura         │  ← Feature (enabled - blue accent bar)
│  ESP              │  ← Feature (disabled)
│  Velocity         │
└─────────────────────┘

┌─────────────────────┐
│ VISUAL          [×] │
├─────────────────────┤
│■ Tracers           │
│  Chams            │
│  Nametags         │
└─────────────────────┘
```

### Interaction
- **Click to toggle** - Click on a feature name to enable/disable
- **Draggable panels** - Click and drag the header to move panel
- **Smooth animations** - Hover effects and transitions
- **Real-time feedback** - Instant visual response to clicks

## Implementation in Our Mod

### Current Status
✅ Basic HUD rendering  
✅ Vape color scheme  
✅ Feature toggles  
🔄 Click detection (in progress)  
🔄 Panel dragging (planned)  
🔄 Animations (planned)

### Color Reference

```java
// Vape v4 Color Palette
private static final int BACKGROUND_DARK = 0x00121212;    // Almost black
private static final int PANEL_BG = 0xFF1A1A1A;           // Slightly lighter
private static final int HEADER = 0xFF1A1A1A;             // Header dark
private static final int ACCENT_BLUE = 0xFF3E80FF;        // Vape blue
private static final int TEXT_ENABLED = 0xFFFFFFFF;       // White
private static final int TEXT_DISABLED = 0xFF6E6E73;      // Gray
private static final int SEPARATOR = 0xFF262626;          // Dark gray
private static final int HOVER = 0xFF1E1E1E;              // Slightly lighter gray
```

### Feature Toggle Implementation

The basic toggle is already implemented in `ESPModClient.java`:

```java
public static void toggleKillAura() {
    killAura = !killAura;
    System.out.println("[ESP Mod] Kill Aura: " + killAura);
}
```

When a feature is toggled:
1. State changes (`killAura = !killAura`)
2. Console logs the change
3. HUD immediately reflects the change (blue accent bar appears)

### Click Detection (TODO)

To implement click-to-toggle, we need to:

```java
private static void handleMouseInput() {
    // Get mouse position (needs to be tracked)
    // Check which panel/feature the mouse is over
    // If clicked, toggle the corresponding feature
}
```

Example implementation:
```java
int mouseX = client.mouse.getX();
int mouseY = client.mouse.getY();

for (Panel panel : panels) {
    for (int i = 0; i < panel.features.length; i++) {
        int featureY = panel.y + HEADER_HEIGHT + (i * ROW_HEIGHT);
        
        if (mouseX >= panel.x && mouseX < panel.x + PANEL_WIDTH &&
            mouseY >= featureY && mouseY < featureY + ROW_HEIGHT) {
            // Feature is being hovered - detect click
            if (mouseClick) {
                // Toggle feature
            }
        }
    }
}
```

### Adding Vape-Like Features

#### 1. Add to Feature List
```java
// In ESPModClient.java
public static volatile boolean myNewFeature = false;
public static void toggleMyNewFeature() {
    myNewFeature = !myNewFeature;
}
```

#### 2. Add to HUD
```java
// In HudRenderer.init()
panels = new Panel[]{
    new Panel("COMBAT", 10, 10, new String[]{
        "Kill Aura",
        "My New Feature"  // Add here
    }),
    // ...
};
```

#### 3. Sync State
```java
// In HudRenderer.syncFeatureStates()
if (panels[0].features.length > 1) {
    panels[0].enabled[1] = ESPModClient.myNewFeature;
}
```

### Advanced Vape Features

#### Sliders
```java
// For numeric values like speed, range, etc.
// Would need extended GUI system
```

#### Keybinds
```java
// Register custom keybinds
// Allow features to be toggled with custom keys
```

#### Module Descriptions
```java
// Show tooltips on hover
// Display module description/info
```

#### Settings Per Module
```java
// Some modules have sub-settings
// E.g., Kill Aura might have:
// - Speed slider
// - Range slider
// - Click type (single/multi)
```

## Styling Reference Code

### Panel Drawing
```java
// Header background
fill(context, panel.x, panel.y, panel.x + PANEL_WIDTH, panel.y + HEADER_HEIGHT, HEADER_BG);

// Left accent bar (appears when enabled)
if (enabled) {
    fill(context, panel.x, y, panel.x + 2, y + ROW_HEIGHT, ACCENT);
}

// Hover effect
if (hovering) {
    fill(context, panel.x, y, panel.x + PANEL_WIDTH, y + ROW_HEIGHT, HOVER_COLOR);
}
```

### Text Rendering
```java
int textColor = enabled ? TEXT_ENABLED : TEXT_DISABLED;
context.drawText(client.textRenderer, name, x, y, textColor, false);
```

## Comparison with Other Clients

### Vape v4
- ✅ Professional dark theme
- ✅ Minimal, clean design
- ✅ Fast and responsive
- ✅ Keyboard shortcuts
- ✅ Customizable colors

### Our Implementation
- ✅ Very similar visual style
- ✅ Same color scheme
- ✅ Same panel layout
- 🔄 Click interaction (can be added)
- 🔄 Keyboard binds (can be added)

## Roadmap

### Phase 1: Basic HUD (✅ DONE)
- Render panels
- Display features
- Vape-style colors
- Basic toggling

### Phase 2: Interactivity (🔄 IN PROGRESS)
- Click-to-toggle
- Hover effects
- Smooth transitions

### Phase 3: Advanced (📋 PLANNED)
- Draggable panels
- Customizable colors
- Module descriptions
- Keybind customization

### Phase 4: Extended (📋 FUTURE)
- Settings per module
- Sliders and inputs
- Categories collapsing
- Save/load configs

## Example: Adding "Tracers" Feature

1. **Add to ESPModClient.java**:
```java
public static volatile boolean tracers = false;
public static void toggleTracers() {
    tracers = !tracers;
}
```

2. **Add to HudRenderer panels**:
```java
new Panel("VISUAL", 220, 10, new String[]{"ESP", "Tracers", "Chams"})
```

3. **Sync state**:
```java
if (panels[1].features.length > 1) {
    panels[1].enabled[1] = ESPModClient.tracers;
}
```

4. **Implement rendering** (in a new mixin):
```java
@Mixin(GameRenderer.class)
public class TracersMixin {
    @Inject(method = "render", at = @At("TAIL"))
    private void onRender(CallbackInfo ci) {
        if (ESPModClient.tracers) {
            // Draw tracers to entities
        }
    }
}
```

## Performance Tips

- Only render when menu is open
- Use hardware-accelerated rendering
- Cache colors instead of creating new objects
- Minimize text rendering calls

## Testing

To test the mod:
1. Build with `./gradlew build`
2. Copy JAR to mods folder
3. Launch Minecraft with Fabric profile
4. Press INSERT to open menu
5. Features should render and toggle

---

**Goal**: Create a professional client similar to Vape v4  
**Status**: Core UI ✅, Features ✅, Interactions 🔄  
**Target**: Fully featured client mod
