# Két Image Viewer agent rules

- Scope: the standalone Qt Widgets library and its gallery demo. Do not modify KétChat/KétSuite or other repositories unless the task explicitly includes them.
- Preserve the independently written MIT implementation. Reference other viewers for behavior; do not copy their source or assets into this repo.
- Before editing, verify the absolute checkout and branch. Preserve unrelated changes.
- Read README.md and the relevant API contract before changing public behavior.
- Keep network/auth/storage and global styling in the host. All widget API calls are GUI-thread calls.
- Run focused tests for affected contracts. Use `ctest --test-dir build/dev -R '^image_viewer_contract$' --output-on-failure`; do not run unrelated deployments' suites.
- For export/package changes, build the installed-package consumer. For visual changes, inspect native renders at an affected normal/narrow size and theme.
- Report code, tests, build and packaging separately. Do not claim cross-platform validation or a public release from a local macOS build.
