# GitHub Actions CI/CD Setup Guide

## Overview

AY Pilot Native uses GitHub Actions for continuous integration and continuous deployment (CI/CD). This guide explains the workflows, how to configure them, and how to troubleshoot issues.

---

## Workflows

### 1. **CI/CD Pipeline** (`ci-cd.yml`)

**Triggers:**
- Push to `main` or `develop` branches
- Pull requests to `main` or `develop` branches
- Manual trigger via `workflow_dispatch`

**Jobs:**
- **C++ Tests** — Builds and tests the shared voice engine
- **Android Build** — Builds APK, AAB, and runs unit tests
- **Desktop Linux** — Builds Electron app for Linux
- **Desktop macOS** — Builds Electron app for macOS
- **Desktop Windows** — Builds Electron app for Windows
- **Code Quality** — Runs ESLint and Prettier
- **Security** — Scans for vulnerabilities
- **Release** — Creates GitHub releases (on version tags)

**Artifacts:**
- Android APK and App Bundle
- Desktop installers (Linux, macOS, Windows)
- Test results and coverage reports

---

### 2. **Pull Request Checks** (`pull-request.yml`)

**Triggers:**
- Pull requests to `main` or `develop` branches

**Jobs:**
- **Validate PR** — Checks PR title format and conflicts
- **Test** — Runs C++ tests
- **Code Review** — Linting and formatting checks
- **Docs** — Validates documentation updates
- **Security** — Security scanning
- **Summary** — Summarizes all checks

**Requirements for PR Merge:**
- ✅ PR title follows conventional commits format
- ✅ All tests pass
- ✅ Code quality checks pass
- ✅ No security vulnerabilities

---

### 3. **Nightly Build** (`nightly.yml`)

**Triggers:**
- Daily at 2 AM UTC
- Manual trigger via `workflow_dispatch`

**Jobs:**
- **Nightly Build** — Comprehensive C++ build with memory leak detection
- **Android Nightly** — Debug build with instrumented tests
- **Desktop Nightly** — Debug build with coverage reports
- **Dependency Check** — Checks for outdated dependencies

**Purpose:**
- Catch edge cases and regressions
- Monitor memory leaks and performance
- Track dependency updates

---

## Configuration

### Required Secrets

Add these secrets to your GitHub repository (Settings → Secrets and variables → Actions):

#### Apple Signing (for macOS builds)
```
APPLE_ID              # Your Apple ID email
APPLE_PASSWORD        # App-specific password
APPLE_TEAM_ID         # Apple Team ID
```

#### Windows Signing (for Windows builds)
```
WINDOWS_CERT          # Base64-encoded .pfx certificate
WINDOWS_CERT_PASSWORD # Certificate password
```

#### Slack Notifications (optional)
```
SLACK_WEBHOOK_URL     # Slack webhook URL for notifications
```

#### Security Scanning (optional)
```
SNYK_TOKEN            # Snyk security scanning token
```

### Adding Secrets

1. Go to your GitHub repository
2. Click **Settings** → **Secrets and variables** → **Actions**
3. Click **New repository secret**
4. Add secret name and value
5. Click **Add secret**

---

## Local Development

### Running Workflows Locally

Use [act](https://github.com/nektos/act) to run workflows locally:

```bash
# Install act
brew install act  # macOS
# or
choco install act  # Windows
# or
sudo apt-get install act  # Linux

# Run a specific workflow
act -j cpp-tests

# Run with specific event
act -e pull_request

# Run with secrets
act -s SLACK_WEBHOOK_URL=https://...
```

### Testing Workflow Changes

1. Create a new branch
2. Modify workflow files in `.github/workflows/`
3. Push to GitHub
4. Create a pull request
5. Workflows run automatically on PR

---

## Troubleshooting

### Workflow Not Running

**Problem:** Workflow doesn't trigger on push

**Solutions:**
1. Check workflow file is in `.github/workflows/` directory
2. Verify branch name matches trigger condition
3. Check file syntax (YAML formatting)
4. Ensure workflow is enabled (Settings → Actions)

### Build Failures

**Problem:** Build fails in CI but works locally

**Solutions:**
1. Check environment differences (OS, tool versions)
2. Review build logs in GitHub Actions
3. Run `act` locally to replicate environment
4. Check for missing dependencies

### Test Failures

**Problem:** Tests pass locally but fail in CI

**Solutions:**
1. Check for environment-specific issues
2. Review test logs in GitHub Actions
3. Check for timing-dependent tests
4. Verify test data is available

### Artifact Upload Issues

**Problem:** Artifacts not uploading

**Solutions:**
1. Check artifact path is correct
2. Verify file exists before upload
3. Check artifact size limits (5GB per artifact)
4. Review upload-artifact action logs

---

## Best Practices

### Workflow Optimization

1. **Use matrix strategy** for multiple OS/version combinations
2. **Cache dependencies** to speed up builds
3. **Parallelize jobs** where possible
4. **Use conditional steps** to skip unnecessary work
5. **Set appropriate timeouts** to catch hanging builds

### Security

1. **Never commit secrets** to repository
2. **Use GitHub Secrets** for sensitive data
3. **Review workflow permissions** regularly
4. **Limit workflow access** to necessary branches
5. **Audit workflow logs** for sensitive information

### Maintenance

1. **Keep workflows updated** with latest actions
2. **Monitor deprecated actions** and migrate
3. **Review workflow performance** regularly
4. **Update tool versions** (NDK, JDK, Node.js)
5. **Document custom steps** for team members

---

## Advanced Configuration

### Custom Build Flags

Edit workflow files to add custom build flags:

```yaml
- name: Build with custom flags
  run: |
    cd android
    ./gradlew build \
      -Dorg.gradle.jvmargs="-Xmx4g" \
      --parallel \
      --build-cache
```

### Conditional Steps

Run steps only on specific conditions:

```yaml
- name: Notify on failure
  if: failure()
  run: echo "Build failed!"

- name: Deploy to production
  if: startsWith(github.ref, 'refs/tags/v')
  run: ./deploy.sh
```

### Matrix Builds

Build across multiple configurations:

```yaml
strategy:
  matrix:
    os: [ubuntu-latest, macos-latest, windows-latest]
    java-version: ['11', '17']
```

---

## Monitoring

### View Workflow Runs

1. Go to repository
2. Click **Actions** tab
3. Select workflow to view runs
4. Click run to see details

### Download Artifacts

1. Go to workflow run
2. Scroll to **Artifacts** section
3. Click artifact to download

### View Logs

1. Go to workflow run
2. Click job name to expand
3. Click step to view logs
4. Search logs for errors

---

## Integration with Other Tools

### Slack Notifications

Workflow automatically sends Slack notifications (if webhook configured):

```yaml
- name: Notify Slack
  uses: slackapi/slack-github-action@v1
  with:
    payload: |
      {
        "text": "Build: ${{ job.status }}"
      }
```

### Code Coverage

Upload coverage reports to Codecov:

```yaml
- name: Upload coverage
  uses: codecov/codecov-action@v3
  with:
    files: ./coverage/coverage-final.json
```

### Release Management

Automatically create releases on version tags:

```yaml
- name: Create Release
  uses: softprops/action-gh-release@v1
  with:
    files: |
      dist/*.apk
      dist/*.dmg
      dist/*.exe
```

---

## Troubleshooting Checklist

- [ ] Workflow file is in `.github/workflows/` directory
- [ ] YAML syntax is valid (use YAML linter)
- [ ] Branch name matches trigger condition
- [ ] Secrets are configured in repository settings
- [ ] Workflow is enabled in Actions settings
- [ ] Tool versions match local development
- [ ] Dependencies are cached properly
- [ ] Artifact paths are correct
- [ ] Permissions are set correctly
- [ ] Logs are reviewed for errors

---

## Support

For issues or questions:

1. Check [GitHub Actions documentation](https://docs.github.com/en/actions)
2. Review workflow logs in GitHub Actions
3. Search [GitHub Actions issues](https://github.com/actions/issues)
4. Open issue in AY Pilot Native repository

---

**Last Updated:** 2026-07-29  
**Status:** Production Ready
