# Third-party software

- **Classic cburnett chess pieces**, by Colin M. L. Burnett, GPL-2.0-or-later.
  The twelve SVG source assets used by Lichess are bundled in `ui/pieces/cburnett/`,
  with their license, attribution, and pinned source revision in that directory.
- **JSON for Modern C++ 3.11.3**, copyright 2013–2023 Niels Lohmann, MIT license.
  The official single header is vendored as `native/vendor/json.hpp`; its license
  is in `native/vendor/LICENSE.json`.
  Source: <https://github.com/nlohmann/json/tree/v3.11.3>.
- **Electron**, MIT license, with Chromium and other components under their
  respective licenses. The installed distribution includes `LICENSE` and
  `LICENSES.chromium.html`; retain these when distributing an application.
- **TypeScript**, Apache-2.0 license, used to compile the application.
- **@types/node**, MIT license, used for build-time type definitions.

Exact npm versions and transitive dependencies are recorded in `package-lock.json`.
Their license files are included with the installed packages in `node_modules/`.
