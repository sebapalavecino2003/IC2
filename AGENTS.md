<!-- GSD:project-start source:PROJECT.md -->
## Project

**NodeAlert IoT**

NodeAlert IoT es un sistema distribuido de monitoreo ambiental crítico para detección temprana de incendios, fugas de gases y automatización preventiva. Combina nodos ESP32 con sensores ambientales (DHT22, MQ-9, KY-026) que procesan datos localmente mediante FreeRTOS, un gateway central con comunicación MQTT, un backend Django en Raspberry Pi con Docker Compose, y un dashboard web en tiempo real con React + Vite + Material UI. Orientado a entornos industriales, residenciales e infraestructura crítica.

**Core Value:** Detectar condiciones ambientales peligrosas (incendio, gas, temperatura extrema) y actuar preventivamente antes de que escalen a emergencias, incluso sin conexión al servidor central.

### Constraints

- **Tech Stack**: ESP32 + FreeRTOS + PlatformIO para firmware; Django + MySQL para backend; React + Vite + MUI para frontend; Docker Compose en Raspberry Pi.
- **Comunicación**: MQTT como protocolo único entre nodos y servidor en v1.
- **Automatización**: Híbrida — los nodos actúan localmente por defecto; el servidor puede enviar comandos de override.
- **Alimentación**: USB/transformador. No hay restricciones de consumo energético en v1.
<!-- GSD:project-end -->

<!-- GSD:stack-start source:STACK.md -->
## Technology Stack

Technology stack not yet documented. Will populate after codebase mapping or first phase.
<!-- GSD:stack-end -->

<!-- GSD:conventions-start source:CONVENTIONS.md -->
## Conventions

Conventions not yet established. Will populate as patterns emerge during development.
<!-- GSD:conventions-end -->

<!-- GSD:architecture-start source:ARCHITECTURE.md -->
## Architecture

Architecture not yet mapped. Follow existing patterns found in the codebase.
<!-- GSD:architecture-end -->

<!-- GSD:skills-start source:skills/ -->
## Project Skills

No project skills found. Add skills to any of: `.claude/skills/`, `.agents/skills/`, `.cursor/skills/`, `.github/skills/`, or `.codex/skills/` with a `SKILL.md` index file.
<!-- GSD:skills-end -->

<!-- GSD:workflow-start source:GSD defaults -->
## GSD Workflow Enforcement

Before using Edit, Write, or other file-changing tools, start work through a GSD command so planning artifacts and execution context stay in sync.

Use these entry points:
- `/gsd-quick` for small fixes, doc updates, and ad-hoc tasks
- `/gsd-debug` for investigation and bug fixing
- `/gsd-execute-phase` for planned phase work

Do not make direct repo edits outside a GSD workflow unless the user explicitly asks to bypass it.
<!-- GSD:workflow-end -->



<!-- GSD:profile-start -->
## Developer Profile

> Profile not yet configured. Run `/gsd-profile-user` to generate your developer profile.
> This section is managed by `generate-claude-profile` -- do not edit manually.
<!-- GSD:profile-end -->
