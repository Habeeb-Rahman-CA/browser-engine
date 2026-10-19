# Security Model and Threat Policy

This document defines the security boundaries, threat model, and mitigation strategies implemented in the educational browser engine.

---

## 1. Threat Model & Purpose

> [!WARNING]
> This browser engine is an educational implementation developed to explore browser design concepts. It is **not** hardened for adversarial, production-grade web browsing on arbitrary untrusted websites.

All external inputs—including HTML documents, stylesheets, JavaScript source files, remote URLs, and images—are treated as potentially untrusted and malformed.

---

## 2. Security Invariants

### Memory Safety & Boundary Enforcement
1. **Zero Unchecked Indexing**: All buffer accesses and string slices must be bound-checked.
2. **RAII Resource Management**: All heap resources, window contexts, and file descriptors are encapsulated in RAII wrappers.
3. **Continuous Sanitizer Verification**: Builds are verified for cleanliness under AddressSanitizer (ASan) and UndefinedBehaviorSanitizer (UBSan).

### Network & URL Handling
1. **Strict Scheme Whitelisting**: Only `http://`, `https://`, and `file://` schemes are supported. Dangerous schemes (e.g. `javascript:`, `data:` execution, `shell:`) are rejected.
2. **Mandatory TLS Verification**: When fetching HTTPS resources via libcurl, certificate validation must never be bypassed or disabled.
3. **Timeouts & Resource Bounds**: Network connections enforce explicit read/connect timeouts and upper limits on resource payload sizes to prevent Denial of Service (DoS) memory exhaustion.

### Script Execution Sandboxing
1. **Instruction & Execution Bounds**: The custom JavaScript interpreter enforces instruction execution limits and recursion depth limits to prevent infinite loops from hanging the browser.
2. **No Direct Host OS Access**: Page scripts have no direct access to system calls, native process execution, or unrestricted local filesystem APIs.
3. **Controlled DOM Exposure**: JavaScript interactions with the browser are mediated exclusively through explicitly registered host bindings.

