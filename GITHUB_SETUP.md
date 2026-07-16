# GitHub Setup - Cloud Build with GitHub Actions

This guide explains how to set up GitHub Actions to build your Fabric mod in the cloud and get a ready-to-use JAR file.

## Why GitHub Actions?

✅ **Free tier** - 2,000 free build minutes/month  
✅ **Compatible environment** - Ubuntu with Java 17  
✅ **Automatic builds** - Every push triggers a build  
✅ **Easy distribution** - Download JAR from artifacts  
✅ **No local setup** - No Gradle/Java compatibility issues  

---

## Step 1: Create GitHub Repository

### Option A: Create New Repository (Recommended)

1. Go to https://github.com/new
2. Enter repository name: `minecraft-esp-mod` (or any name)
3. Select **Public** or **Private**
4. Click "Create repository"
5. Follow the instructions to push your code

### Option B: Use Existing Repository

If you already have a GitHub account and repo, skip to Step 2.

---

## Step 2: Push Your Code to GitHub

### From Command Line (Windows)

```bash
cd C:\Users\Rdp1\Desktop\Mincraft

# Initialize git if not already done
git init

# Add your GitHub repository as remote
git remote add origin https://github.com/YOUR_USERNAME/minecraft-esp-mod.git

# Stage all files
git add .

# Commit
git commit -m "Initial commit: Vape v4-style Fabric mod"

# Push to GitHub (might prompt for authentication)
git push -u origin main
```

**Note**: You'll need to:
1. Create a Personal Access Token at https://github.com/settings/tokens
2. Use it as password when prompted
3. Or use Git for Windows with SSH keys

---

## Step 3: Verify GitHub Actions Setup

1. Go to your repository on GitHub
2. Click the "Actions" tab
3. You should see a workflow called "Build Fabric Mod"
4. The workflow file is: `.github/workflows/build.yml`

---

## Step 4: Trigger a Build

### Option A: Push Code (Automatic)

Simply pushing code to `main` or `master` branch will trigger the build:

```bash
# Make a change, commit, and push
git add .
git commit -m "Update: Add new feature"
git push
```

### Option B: Manual Trigger

1. Go to GitHub repository
2. Click "Actions" tab
3. Select "Build Fabric Mod" workflow
4. Click "Run workflow" button
5. Select branch (main)
6. Click "Run workflow"

---

## Step 5: Download the JAR

### Where to Find It

1. Go to your repository on GitHub
2. Click "Actions" tab
3. Click the latest successful build
4. Scroll down to "Artifacts"
5. Click "espmod-jar" to download
6. Extract the ZIP file
7. You'll find `espmod-1.0.0.jar`

---

## Step 6: Install the JAR

Once you have the JAR file:

```bash
# Copy to Minecraft mods folder
copy espmod-1.0.0.jar %APPDATA%\.minecraft\mods\

# Launch Minecraft (with Fabric profile)
# Press INSERT in-game to see the HUD!
```

---

## How GitHub Actions Works

```
You push code to GitHub
         ↓
GitHub Actions detects change
         ↓
Starts Ubuntu virtual machine
         ↓
Sets up Java 17
         ↓
Runs: gradlew build
         ↓
Compiles Fabric mod
         ↓
Creates JAR file
         ↓
Uploads to Artifacts
         ↓
You download the JAR
         ↓
Install in Minecraft
         ↓
Play!
```

---

## Build Status Indicators

### ✅ Successful Build
- Green checkmark on GitHub
- JAR available in Artifacts
- Ready to download and use

### ❌ Failed Build
- Red X on GitHub
- Click on build to see error logs
- Common issues:
  - Syntax errors in Java code
  - Missing dependencies
  - Gradle configuration issues

---

## Making Changes & Rebuilding

The beauty of GitHub Actions is **automatic rebuilds**:

1. **Make a change** to your Java code
2. **Commit and push** to GitHub
3. **GitHub Actions** automatically rebuilds
4. **New JAR** is available in Artifacts

No need to deal with local build issues!

---

## Custom Build Configuration

### Change Build Trigger

Edit `.github/workflows/build.yml` to trigger on:

```yaml
on:
  push:
    branches: [ main, master, develop ]  # Also build on develop
  schedule:
    - cron: '0 0 * * 0'  # Weekly builds
```

### Change Java Version

```yaml
- name: Set up JDK 21  # Use Java 21 instead of 17
  uses: actions/setup-java@v3
  with:
    java-version: '21'
```

### Add Build Notifications

```yaml
- name: Notify on Slack
  if: failure()
  uses: slackapi/slack-github-action@v1
  # ... configure your Slack webhook
```

---

## Troubleshooting

### Build Fails with "Gradle not found"

The workflow includes `chmod +x gradlew` to make it executable. If this fails:

```yaml
# Already in the workflow, but you can check:
- name: Make gradlew executable
  run: chmod +x fabric-mod/gradlew
```

### Build Fails with "Java version"

Check the workflow is using JDK 17+:

```yaml
java-version: '17'  # Minimum for Fabric mod
```

### JAR Not Found in Artifacts

Check the path in the workflow:

```yaml
path: fabric-mod/build/libs/espmod-*.jar
```

This must match where your build outputs the JAR.

---

## Advanced: Release Automation

The workflow automatically creates GitHub Releases when you tag a commit:

```bash
# Tag a version
git tag v1.0.0

# Push the tag
git push origin v1.0.0
```

GitHub Actions will:
1. Build the mod
2. Create a Release
3. Attach the JAR as a downloadable asset

---

## CI/CD Pipeline Summary

Your setup now has:

✅ **Continuous Integration**
- Auto-builds on every push
- Tests run automatically
- Status shown on GitHub

✅ **Continuous Deployment**
- JAR uploaded to Artifacts
- Easy to download
- No manual build steps

✅ **Release Automation**
- Tag a commit → automatic release
- JAR available for download
- Version tracked automatically

---

## Example Workflow

```
Day 1: Create repository with GitHub Actions
           ↓
Day 2: Make code changes locally
           ↓
       git push to GitHub
           ↓
       GitHub Actions auto-builds
           ↓
       Download JAR from Artifacts
           ↓
       Test in Minecraft
           ↓
Day 3: Find a bug, fix it
           ↓
       git push again
           ↓
       New JAR automatically built
           ↓
       Download and test
           ↓
       Repeat forever!
```

---

## No More Local Build Issues!

With GitHub Actions, you never need to:
- ❌ Install Java locally
- ❌ Configure Gradle
- ❌ Deal with version conflicts
- ❌ Build locally at all

Just code, push, and download! ✅

---

## Next Steps

1. ✅ Create a GitHub account (if needed)
2. ✅ Create a new repository
3. ✅ Push this code to GitHub
4. ✅ Watch GitHub Actions build it
5. ✅ Download the JAR
6. ✅ Install and play!

---

## Resources

- GitHub Actions Docs: https://docs.github.com/en/actions
- GitHub Actions Java: https://github.com/actions/setup-java
- Gradle GitHub Actions: https://github.com/gradle/gradle-build-action

---

**Status**: 🟢 Ready to go!  
**Build Time**: ~5-10 minutes (first build), ~2-3 minutes (subsequent)  
**Cost**: FREE on GitHub  

Let's build! 🚀
