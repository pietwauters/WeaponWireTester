# Contributing to WeaponWireTester

Thanks for your interest in contributing! This project combines **software**, **hardware design files**, and **documentation**. To keep things clear and legally tidy, please read this document before opening issues or pull requests.

**Repository:** https://github.com/pietwauters/WeaponWireTester  
**Licensors:** © 2022–2025 Piet Wauters & Claude Wolter

---

## 1) Ground rules & licensing (IMPORTANT)

By submitting a contribution (issues, PRs, code, docs, or hardware design files), you agree that:

- **Code (software)** you contribute is licensed under **Apache License 2.0**.
  - Add: `SPDX-License-Identifier: Apache-2.0` at the top of new source files.

- **Hardware design files & documentation** you contribute are licensed under **CC BY-NC-SA 4.0** and you accept the project’s NonCommercial interpretation:
  - **Producing more than 10 units** derived from these design files (including remixes/derivatives), *whether sold or given away*, is **commercial use** and **requires a separate commercial license**.
  - Add (in KiCad title blocks, README headers, etc.):  
    `© 2022–2025 Piet Wauters & Claude Wolter — CC BY-NC-SA 4.0 — Commercial use (>10 units, including remixes) by separate agreement.`

- **Developer Certificate of Origin (DCO)** — You certify you have the right to submit the contribution under these terms by including a **Signed-off-by** line in each commit:
- Signed-off-by: Your Name you@example.com

Use `git commit -s` to add this automatically. See https://developercertificate.org/

- **Third-party IP** — Don’t submit content you can’t relicense as above. If you include third-party materials, state their original license and source clearly.

- **Trademarks/logos** — Project logos/marks are **not** covered by the open licenses unless a specific notice says otherwise. Don’t use or modify them without permission.

For details, see:
- `LICENSE` (Apache-2.0 for code)
- `LICENSE-CC-BY-NC-SA-4.0.md` (hardware/docs + >10-unit policy)
- `LICENSE-MAP.md` (what applies where)
- `COMMERCIAL-LICENSING.md` (how to request a commercial license)

---

## 2) How to contribute

### Report a bug
1. Search existing issues to avoid duplicates.
2. Open a new issue with:
 - **What happened** vs. **what you expected**
 - **Steps to reproduce** (screenshots/logs welcome)
 - **Environment** (OS, tool versions, board rev, etc.)
 - For hardware: PCB rev, fab house, assembly notes, components affected.
 - For software: commit hash, toolchain, board target, serial logs.

### Propose an enhancement
1. Open an issue tagged **enhancement** describing:
 - The problem/use-case
 - Proposed solution or alternatives
 - Impact/risk and testing ideas

### Submit a pull request (PR)
1. Fork → create a feature branch.
2. Keep PRs focused and small when possible.
3. Follow the checklists below (software / hardware).
4. Use **Conventional Commits** for commit messages (e.g., `feat:`, `fix:`, `docs:`, `chore:`, `refactor:`). See https://www.conventionalcommits.org/
5. Ensure all CI checks pass.

---

## 3) PR checklists

### Software PR checklist
- [ ] Code compiles and runs on supported targets.
- [ ] `SPDX-License-Identifier: Apache-2.0` in new source files.
- [ ] Unit/functional tests added or updated (where applicable).
- [ ] No secrets, API keys, or personal data committed.
- [ ] README or inline docs updated if behavior changes.
- [ ] Reasonable code style (see §4).

### Hardware/Docs PR checklist
- [ ] Files licensed/marked **CC BY-NC-SA 4.0** (title block/footer/README).
- [ ] KiCad source included (`.kicad_pro`, `.kicad_sch`, `.kicad_pcb`, custom libs).
- [ ] **Version & revision** clearly indicated in the schematic title block and PCB fab notes (e.g., `Rev A2`).
- [ ] **ERC/DRC** run clean or with documented waivers in the PR description.
- [ ] Manufacturing outputs attached or reproducible (Gerbers/drill, pick-and-place, PDF schematic, STEP/3D if changed).
- [ ] **BOM** updated (format: CSV + optional interactive BOM).
- [ ] Mechanical constraints respected (board outline, keep-outs, hole sizes).
- [ ] Changes documented in `docs/` (what changed and why).
- [ ] If modifying licensed third-party footprints/models, retain their notices.

---

## 4) Software: style & tooling

- **Languages/targets:** ESP32/embedded C/C++ (Arduino/PlatformIO or IDF as applicable).
- **Formatting:** Prefer `clang-format` defaults or a project `.clang-format` if present.
- **Headers:** Add SPDX and a short file header comment (purpose, module owner).
- **Dependencies:** Keep them minimal; update lockfiles if relevant.
- **Logging:** Prefer structured logs with levels (`DEBUG/INFO/WARN/ERROR`).
- **Tests:** Add unit/integration tests where feasible; avoid flaky sleeps/timeouts.
- **Performance:** Avoid busy loops; use events/interrupts where appropriate.
- **Security:** Avoid hard-coded credentials; validate inputs rigorously.

**SPDX example (code):**
```c
//SPDX-License-Identifier: Apache-2.0
```


## 5) Hardware: tools, versions & conventions

- **ECAD tool:** KiCad **9.x** (please do not down-rev files).
- **File types to include:**
  - Project: `.kicad_pro`, `.kicad_sch`, `.kicad_pcb`
  - Libraries: custom symbols/footprints in `hardware/lib/` with `.kicad_sym`, `.kicad_mod` (prefer project-local libs for reproducibility)
  - Outputs (when releasing or for review): Gerbers, drill files, pick-and-place CSV, PDF schematic, STEP/3D if changed
- **Units & grid:** Metric preferred; meaningful grid; rounded coordinates when possible.
- **Naming:** Clear reference designators; net names for critical signals; label test points.
- **Schematics:** Hierarchical sheets OK; keep sheet notes explaining design intent.
- **BOM:** Include manufacturer part numbers and alternates where safe.
- **DRC/ERC:** Run both. Document any intentional violations.
- **Stack-up & clearances:** Note target fab process (e.g., JLCPCB standard rules) in board fab notes.
- **Title block footer (license line):**
© 2022–2025 Piet Wauters & Claude Wolter — CC BY-NC-SA 4.0 — Commercial use (>10 units, including remixes) by agreement


---

## 6) Documentation

- **Format:** Markdown (`.md`). Keep docs in `docs/`.
- **Images:** Prefer SVG (diagrams) or PNG (screenshots). Include alt text.
- **Attribution:** When based on third-party material, credit and link the source.
- **Licensing footer (docs pages):**
© 2022–2025 Piet Wauters & Claude Wolter — CC BY-NC-SA 4.0


---

## 7) Security

- **Do not** file sensitive security issues in public issues.
- Use GitHub’s **private security advisory** workflow (Security → Advisories) or open a minimal issue asking a maintainer to start a private thread.
- Provide reproduction steps, impacted versions, and mitigation ideas if possible.

---

## 8) Code of Conduct

We follow the [Contributor Covenant](https://www.contributor-covenant.org/) (v2.x).  
Please see `CODE_OF_CONDUCT.md` (or open an issue if it’s missing and we’ll add it).

---

## 9) Release packaging (maintainers)

When tagging a release that includes hardware changes:
- Export clean **manufacturing outputs** (Gerbers/drill, PnP, PDF schematic, STEP).
- Publish a **release note** summarizing changes and risks.
- Update **BOM** and **docs**.
- Re-run DRC/ERC and attach summaries.

---

## 10) Commercial licensing

If your intended use is **commercial** (e.g., **>10 units** total including remixes/derivatives; sale, giveaway, pilots, or internal deployment), request terms as described in `COMMERCIAL-LICENSING.md`.

Open an issue titled:
[Commercial License Request] WeaponWireTester


---

## 11) Legal reminders (not legal advice)

- **Apache-2.0** and **CC BY-NC-SA 4.0** are provided **“AS IS”** without warranties; liability is limited to the maximum extent permitted by law.
- Manufacturing/selling physical goods may trigger additional obligations under your local laws regardless of the IP license.

---

## 12) Thank you!

Your contributions make this project better for everyone — thank you for your time and expertise!


