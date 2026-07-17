package com.espmod.ui;

import com.espmod.ESPModClient;
import net.minecraft.client.MinecraftClient;
import net.minecraft.client.gui.DrawContext;

public class HudRenderer {

    private static final MinecraftClient client = MinecraftClient.getInstance();

    // Vape v4 colors
    private static final int PANEL_BG   = 0xCC0D0D0D;
    private static final int HEADER_BG  = 0xFF161616;
    private static final int ACCENT     = 0xFF3E80FF;
    private static final int TEXT_ON    = 0xFFFFFFFF;
    private static final int TEXT_OFF   = 0xFF6E6E73;
    private static final int SEPARATOR  = 0xFF222222;

    // Compact Vape-style sizing
    private static final int PANEL_W   = 110;
    private static final int HEADER_H  = 14;
    private static final int ROW_H     = 14;
    private static final int PAD       = 5;
    private static final int GAP       = 5; // gap between panels

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
        int x = 5;
        panels = new Panel[]{
            new Panel("COMBAT",   x,                           5, new String[]{"Kill Aura"}),
            new Panel("VISUAL",   x + (PANEL_W + GAP),         5, new String[]{"ESP"}),
            new Panel("MOVEMENT", x + (PANEL_W + GAP) * 2,    5, new String[]{"Auto Sprint"})
        };
    }

    public static void render(DrawContext ctx, float tickDelta) {
        if (!ESPModClient.menuOpen) return;
        syncStates();
        for (Panel p : panels) drawPanel(ctx, p);
    }

    public static void handleClick() {
        if (client.getWindow() == null) return;
        double scale = client.getWindow().getScaleFactor();
        int mx = (int)(client.mouse.getX() / scale);
        int my = (int)(client.mouse.getY() / scale);

        for (int pi = 0; pi < panels.length; pi++) {
            Panel p = panels[pi];
            for (int fi = 0; fi < p.features.length; fi++) {
                int fy = p.y + HEADER_H + fi * ROW_H;
                if (mx >= p.x && mx < p.x + PANEL_W && my >= fy && my < fy + ROW_H) {
                    if (pi == 0 && fi == 0) ESPModClient.toggleKillAura();
                    if (pi == 1 && fi == 0) ESPModClient.toggleESP();
                    if (pi == 2 && fi == 0) ESPModClient.toggleAutoSprint();
                }
            }
        }
    }

    private static void drawPanel(DrawContext ctx, Panel p) {
        int totalH = HEADER_H + p.features.length * ROW_H;

        // Background
        ctx.fill(p.x, p.y + HEADER_H, p.x + PANEL_W, p.y + totalH, PANEL_BG);

        // Header
        ctx.fill(p.x, p.y, p.x + PANEL_W, p.y + HEADER_H, HEADER_BG);

        // Top accent line
        ctx.fill(p.x, p.y, p.x + PANEL_W, p.y + 1, ACCENT);

        // Left accent bar
        ctx.fill(p.x, p.y, p.x + 1, p.y + totalH, ACCENT);

        // Header text
        ctx.drawText(client.textRenderer, p.title, p.x + PAD, p.y + 3, TEXT_ON, false);

        // Feature rows
        for (int i = 0; i < p.features.length; i++) {
            int fy = p.y + HEADER_H + i * ROW_H;
            drawFeature(ctx, p, i, fy);
        }
    }

    private static void drawFeature(DrawContext ctx, Panel p, int idx, int y) {
        boolean on   = p.enabled[idx];
        String  name = p.features[idx];

        double scale = client.getWindow().getScaleFactor();
        int mx = (int)(client.mouse.getX() / scale);
        int my = (int)(client.mouse.getY() / scale);
        boolean hover = mx >= p.x && mx < p.x + PANEL_W && my >= y && my < y + ROW_H;

        // Hover highlight
        if (hover) ctx.fill(p.x + 1, y, p.x + PANEL_W, y + ROW_H, 0x221E1E1E);

        // Enabled: blue left bar
        if (on) ctx.fill(p.x + 1, y, p.x + 3, y + ROW_H, ACCENT);

        // Separator
        if (idx < p.features.length - 1)
            ctx.fill(p.x + 1, y + ROW_H - 1, p.x + PANEL_W, y + ROW_H, SEPARATOR);

        // Feature text
        ctx.drawText(client.textRenderer, name,
                     p.x + PAD + 2, y + 3,
                     on ? TEXT_ON : TEXT_OFF, false);

        // Checkbox
        int cx = p.x + PANEL_W - 14, cy = y + 2, cs = 10;
        ctx.fill(cx, cy, cx + cs, cy + cs, 0xFF111111);
        if (on) ctx.fill(cx + 2, cy + 2, cx + cs - 2, cy + cs - 2, ACCENT);
    }

    private static void syncStates() {
        if (panels == null) return;
        panels[0].enabled[0] = ESPModClient.isKillAuraEnabled();
        panels[1].enabled[0] = ESPModClient.isESPEnabled();
        panels[2].enabled[0] = ESPModClient.isAutoSprintEnabled();
    }
}
