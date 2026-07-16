# ⚡ Quick Start: Build JAR with GitHub Actions (5 Minutes)

## TL;DR - Do This Now

### 1. Sign Up for GitHub (Free)
Go to https://github.com/join (1 minute)

### 2. Create Repository
- Go to https://github.com/new
- Name it: `minecraft-esp-mod`
- Click "Create repository" (1 minute)

### 3. Push Your Code
```bash
cd C:\Users\Rdp1\Desktop\Mincraft

git init
git add .
git commit -m "Initial: Vape v4 Fabric Mod"
git branch -M main
git remote add origin https://github.com/YOUR_USERNAME/minecraft-esp-mod.git
git push -u origin main
```

(2 minutes, might prompt for GitHub login)

### 4. Watch It Build
- Go to your repo on GitHub
- Click "Actions" tab
- Watch the build progress in real-time
- It takes ~5-10 minutes first time

### 5. Download JAR
- When build is done (green ✅)
- Click the build
- Scroll down to "Artifacts"
- Click "espmod-jar" to download
- Extract the ZIP
- Inside is: `espmod-1.0.0.jar` ✅

### 6. Install & Play
```bash
copy espmod-1.0.0.jar %APPDATA%\.minecraft\mods\

# Launch Minecraft (Fabric profile)
# Press INSERT in-game
# See the HUD!
```

---

## That's It! 🎉

Your JAR is built and ready to use!

---

## Detailed Steps Below

### Need Git?

Download Git for Windows: https://git-scm.com/download/win

### Need Help with GitHub?

#### Step 1: Create Account
1. Go to https://github.com/join
2. Enter email, password, username
3. Click "Create account"
4. Verify email
5. Done! ✅

#### Step 2: Create Repository
1. Click "+" in top right
2. Select "New repository"
3. Name: `minecraft-esp-mod`
4. Description: `Vape v4-style Fabric Mod`
5. Select "Public" or "Private"
6. Click "Create repository"
7. Copy the HTTPS URL (looks like: `https://github.com/YOUR_USERNAME/minecraft-esp-mod.git`)

#### Step 3: Push Code
Open Command Prompt in `C:\Users\Rdp1\Desktop\Mincraft`:

```bash
git init
git add .
git commit -m "Initial: Vape v4 Fabric Mod"
git branch -M main
git remote add origin https://github.com/YOUR_USERNAME/minecraft-esp-mod.git
git push -u origin main
```

When it asks for password:
- Username: your GitHub username
- Password: Create a Personal Access Token at https://github.com/settings/tokens
  - Click "Generate new token (classic)"
  - Check "repo" checkbox
  - Click "Generate"
  - Copy the long token string
  - Use that as your password

#### Step 4: Watch Build
1. Go to https://github.com/YOUR_USERNAME/minecraft-esp-mod
2. Click "Actions" tab
3. You should see "Build Fabric Mod" workflow running
4. Click on it to watch progress
5. Wait for green ✅ checkmark

#### Step 5: Download JAR
1. In the "Build Fabric Mod" run, scroll down
2. Look for "Artifacts" section
3. Click "espmod-jar" to download ZIP
4. Extract the ZIP
5. Find `espmod-1.0.0.jar` inside

#### Step 6: Install JAR
```bash
# Copy to Minecraft mods folder
copy espmod-1.0.0.jar %APPDATA%\.minecraft\mods\

# If that doesn't work, navigate manually:
# C:\Users\YourUsername\AppData\Roaming\.minecraft\mods
```

#### Step 7: Test
1. Launch Minecraft (Fabric profile)
2. Create/load a world
3. Press **INSERT** in-game
4. You should see the HUD appear! ✅

---

## Troubleshooting

### "git is not recognized"
Install Git: https://git-scm.com/download/win

### "GitHub username/password rejected"
Use a Personal Access Token instead of password:
1. Go to https://github.com/settings/tokens
2. Click "Generate new token (classic)"
3. Check "repo"
4. Click "Generate token"
5. Copy the token
6. Use it as your password

### Build fails in GitHub Actions
- Click on the failed build
- Look at the error message
- Common fixes:
  - Syntax error in Java code
  - Try running locally to see the exact error
  - Check that all files are committed

### JAR not in Artifacts
- Build may not have completed
- Refresh the page
- Check that build is green ✅
- Artifacts only appear after successful build

### JAR installed but HUD doesn't appear
- Make sure Fabric API is in mods folder
- Press INSERT key
- Check Minecraft logs in `.minecraft/logs/latest.log`
- Mod should log "[ESP Mod]" messages

---

## What Happens Next

**Every time you push code to GitHub:**
1. GitHub Actions automatically builds
2. New JAR created in ~5 minutes
3. Download latest JAR from Artifacts
4. Install and test

**You can now:**
- Make changes locally
- Push to GitHub
- Get a fresh build automatically
- No local compilation needed!

---

## Benefits

✅ **No local build issues** - GitHub has proper environment  
✅ **Fast builds** - Cached dependencies  
✅ **Easy sharing** - Send the JAR to friends  
✅ **Version control** - Track all changes  
✅ **Free** - GitHub Actions is free tier  

---

## That's All!

You now have:
1. ✅ Code on GitHub
2. ✅ Automatic builds
3. ✅ Ready-to-use JAR files
4. ✅ Professional setup

Enjoy your Vape v4 client! 🎮

---

**Build Status**: Check here after pushing: `https://github.com/YOUR_USERNAME/minecraft-esp-mod/actions`

**Download JAR**: After build completes: `https://github.com/YOUR_USERNAME/minecraft-esp-mod/actions` → Click build → Artifacts

---

Questions? Check `GITHUB_SETUP.md` for more details.
