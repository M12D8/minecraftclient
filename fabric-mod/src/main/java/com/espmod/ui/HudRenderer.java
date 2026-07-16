package com.espmod.ui;

import com.espmod.ESPModClient;
import net.minecraft.client.MinecraftClient;
import net.minecraft.client.gui.DrawContext;

/**
 * HUD Renderer - Vape v4 style in-game GUI
 */
public class HudRenderer {

    private static final MinecraftClient client = MinecraftClient.getInstance();

    // Vape v4 color scheme
    private static final int PANEL_BG     = 0xCC121212;
    private static final int HEADER_BG    = 0xFF1A1A1A;
    private static final int ACCENT       = 0xFF3E80FF;
    private static final int TEXT_WHITE   = 0xFFFFFFFF;
    private static final int TEXT_GRAY    = 0xFF6E6E73;
    private static final int SEPARATOR    = 0xFF262626;

    private static final int PANEL_WIDTH   = 200;
    private static final int HEADER_HEIGHT = 20;
    private static final int ROW_HEIGHT    = 18;
    private static final int PADDING       = 8;

    private static class Panel {
        String title;
        int x, y;
        String[] features;
        boolean[] enabled;

        Panel(String title, int x, int y, String[] features) {
            this.title    = title;
            this.x        = x;
            this.y        = y;
            this.features = features;
            this.enabled  = new boolean[features.length];
        }
    }

    private static Panel[] panels;

    public static void init() {
        panels = new Panel[]{
            new Panel("COMBAT",   10,  10, new String[]{"Kill Aura"}),
            new Panel("VISUAL",  220,  10, new String[]{"ESP"}),
            new Panel("MOVEMENT",430,  10, new String[]{"Auto Sprint"})
        };
    }

    public static void render(DrawContext context, float tickDelta) {
        if (!ESPModClient.menuOpen) return;

        syncFeatureStates();

        for (Panel panel : panels) {
            drawPanel(context, panel);
        }
    }

    private static void drawPanel(DrawContext context, Panel panel) {
        int panelHeight = HEADER_HEIGHT + (panel.features.length * ROW_HEIGHT);

        // Background
        context.fill(panel.x, panel.y + HEADER_HEIGHT,
                     panel.x + PANEL_WIDTH, panel.y + panelHeight, PANEL_BG);

        // Header
        context.fill(panel.x, panel.y,
                     panel.x + PANEL_WIDTH, panel.y + HEADER_HEIGHT, HEADER_BG);
        context.fill(panel.x, panel.y + HEADER_HEIGHT - 1,
                     panel.x + PANEL_WIDTH, panel.y + HEADER_HEIGHT, SEPARATOR);

        // Header title
        context.drawText(client.textRenderer, panel.title,
                         panel.x + PADDING, panel.y + 6, 0xFFB9B9C0, false);

        // Left + top accent border
        context.fill(panel.x, panel.y, panel.x + 1, panel.y + panelHeight, ACCENT);
        context.fill(panel.x, panel.y, panel.x + PANEL_WIDTH, panel.y + 1, ACCENT);

        // Feature rows
        for (int i = 0; i < panel.features.length; i++) {
            int featureY = panel.y + HEADER_HEIGHT + (i * ROW_HEIGHT);
            drawFeature(context, panel, i, featureY);
        }
    }

    private static void drawFeature(DrawContext context, Panel panel, int idx, int y) {
        boolean enabled = panel.enabled[idx];
        String  name    = panel.features[idx];

        // Mouse hover detection (GUI coordinates)
        double scale  = client.getWindow().getScaleFactor();
        int mouseX = (int)(client.mouse.getX() / scale);
        int mouseY = (int)(client.mouse.getY() / scale);
        boolean hovering = mouseX >= panel.x && mouseX < panel.x + PANEL_WIDTH
                        && mouseY >= y        && mouseY < y + ROW_HEIGHT;

        if (hovering) {
            context.fill(panel.x, y, panel.x + PANEL_WIDTH, y + ROW_HEIGHT, 0x331E1E1E);
        }

        // Blue left bar when enabled
        if (enabled) {
            context.fill(panel.x, y, panel.x + 2, y + ROW_HEIGHT, ACCENT);
        }

        // Separator
        if (idx < panel.features.length - 1) {
            context.fill(panel.x, y + ROW_HEIGHT - 1,
                         panel.x + PANEL_WIDTH, y + ROW_HEIGHT, SEPARATOR);
        }

        // Feature name
        context.drawText(client.textRenderer, name,
                         panel.x + PADDING, y + 4,
                         enabled ? TEXT_WHITE : TEXT_GRAY, false);

        // Checkbox
        int cx = panel.x + PANEL_WIDTH - 20;
        int cy = y + 3;
        int cs = 12;
        context.fill(cx, cy, cx + cs, cy + cs, 0xFF1A1A1A);
        if (enabled) {
            context.fill(cx + 2, cy + 2, cx + cs - 2, cy + cs - 2, ACCENT);
        }
    }

    private static void syncFeatureStates() {
        if (panels == null) return;
        panels[0].enabled[0] = ESPModClient.isKillAuraEnabled();
        panels[1].enabled[0] = ESPModClient.isESPEnabled();
        panels[2].enabled[0] = ESPModClient.isAutoSprintEnabled();
    }
}
