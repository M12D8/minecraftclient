package com.espmod;

import net.fabricmc.api.ClientModInitializer;
import net.fabricmc.fabric.api.client.event.lifecycle.v1.ClientTickEvents;
import net.fabricmc.fabric.api.client.rendering.v1.HudRenderCallback;
import org.lwjgl.glfw.GLFW;

import net.minecraft.client.MinecraftClient;

public class ESPModClient implements ClientModInitializer {

    private static final MinecraftClient client = MinecraftClient.getInstance();

    public static volatile boolean menuOpen = false;

    // Feature states
    public static volatile boolean killAura = false;
    public static volatile boolean esp = false;
    public static volatile boolean autoSprint = false;

    private static boolean lastInsertPressed = false;

    @Override
    public void onInitializeClient() {
        System.out.println("[ESP Mod] Initializing client...");

        HudRenderer.init();

        // In Fabric API for 1.21, HudRenderCallback passes (DrawContext, RenderTickCounter)
        HudRenderCallback.EVENT.register((drawContext, tickCounter) -> {
            HudRenderer.render(drawContext, tickCounter.getTickDelta(true));
        });

        ClientTickEvents.END_CLIENT_TICK.register(c -> handleInput());

        System.out.println("[ESP Mod] Client initialized!");
    }

    private void handleInput() {
        if (client.getWindow() == null) return;

        long handle = client.getWindow().getHandle();

        int insertState = GLFW.glfwGetKey(handle, GLFW.GLFW_KEY_INSERT);
        if (insertState == GLFW.GLFW_PRESS) {
            if (!lastInsertPressed) {
                menuOpen = !menuOpen;
                System.out.println("[ESP Mod] Menu: " + (menuOpen ? "OPEN" : "CLOSED"));
            }
            lastInsertPressed = true;
        } else {
            lastInsertPressed = false;
        }
    }

    public static boolean isKillAuraEnabled()  { return killAura; }
    public static boolean isESPEnabled()        { return esp; }
    public static boolean isAutoSprintEnabled() { return autoSprint; }

    public static void toggleKillAura()   { killAura   = !killAura; }
    public static void toggleESP()        { esp        = !esp; }
    public static void toggleAutoSprint() { autoSprint = !autoSprint; }
}
