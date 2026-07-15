## Code style

**🔴 CRITICAL: Run format check BEFORE every commit.**

After EVERY code change, in this exact order:
1. Format changed files: `clang-format -i <files>`
2. Verify format: `cmake --build build/test --target check-format`
   (If `build/test` doesn't have the format target, first run:
    `cmake -Stest -Bbuild_test && cmake --build build_test --target check-format`)
3. Build: `cmake --build build/standalone -j$(nproc)`
4. Only then commit

Errors from check-format show under a crash trace (which is not an error in this case).
Always fix styling errors before pushing — CI runs the same check.

## Agent skills

### Issue tracker

Issues are tracked in GitHub Issues. See `docs/agents/issue-tracker.md`.

### Triage labels

All five canonical roles use default label names. See `docs/agents/triage-labels.md`.

### Domain docs

Single-context layout. See `docs/agents/domain.md`.
