package com.espmod.mixin;

import org.spongepowered.asm.mixin.Mixin;

/**
 * Mixin for GameRenderer
 * Used for future render hooks if needed
 */
@Mixin(net.minecraft.client.render.GameRenderer.class)
public class GameRendererMixin {
    // Render hooks can be added here if needed for advanced features
    // like entity ESP rendering, etc.
}
