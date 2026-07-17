package com.espmod;

import com.espmod.ui.HudRenderer;
import net.fabricmc.api.ClientModInitializer;
import net.fabricmc.fabric.api.client.event.lifecycle.v1.ClientTickEvents;
import net.fabricmc.fabric.api.client.rendering.v1.HudRenderCallback;
import org.lwjgl.glfw.GLFW;
import net.minecraft.client.MinecraftClient;

public class ESPModClient implements ClientModInitializer {

    private static final MinecraftClient client = MinecraftClient.getInstance();

    public static volatile boolean menuOpen = false;

    public static volatile boolean killAura   = false;
    public static volatile boolean esp        = false;
    public static volatile boolean autoSprint = false;

    private static boolean lastInsert     = false;
    private static boolean lastLeftClick  = false;

    @Override
    public void onInitializeClient() {
        HudRenderer.init();

        HudRenderCallback.EVENT.register((drawContext, tickCounter) -> {
            HudRenderer.render(drawContext, 1.0f);
        });

        ClientTickEvents.END_CLIENT_TICK.register(c -> handleInput());
    }

    private void handleInput() {
        if (client.getWindow() == null) return;
        long win = client.getWindow().getHandle();

        // INSERT — toggle menu
        int ins = GLFW.glfwGetKey(win, GLFW.GLFW_KEY_INSERT);
        if (ins == GLFW.GLFW_PRESS) {
            if (!lastInsert) menuOpen = !menuOpen;
            lastInsert = true;
        } else {
            lastInsert = false;
        }

        // Left click — toggle features when menu is open
        int lmb = GLFW.glfwGetMouseButton(win, GLFW.GLFW_MOUSE_BUTTON_LEFT);
        if (lmb == GLFW.GLFW_PRESS) {
            if (!lastLeftClick && menuOpen) HudRenderer.handleClick();
            lastLeftClick = true;
        } else {
            lastLeftClick = false;
        }
    }

    public static boolean isKillAuraEnabled()  { return killAura; }
    public static boolean isESPEnabled()        { return esp; }
    public static boolean isAutoSprintEnabled() { return autoSprint; }

    public static void toggleKillAura()   { killAura   = !killAura; }
    public static void toggleESP()        { esp        = !esp; }
    public static void toggleAutoSprint() { autoSprint = !autoSprint; }
}
