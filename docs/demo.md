# Project Demo - Functional Verification

This document provides visual evidence of the Proxy Server's capabilities, including HTTP forwarding, HTTPS tunneling (CONNECT), domain filtering, and real-time metrics.

---

## 1. Basic HTTP Request (Forwarding)

**Command:**
`curl.exe -x localhost:8888 http://example.com`

**Result:**
The proxy successfully intercepted the request, fetched the data from the remote server, and returned the HTML content.

**Output Snippet:**

```html
<!doctype html>
<html>
<head>
    <title>Example Domain</title>
    <meta charset="utf-8" />
    <meta http-equiv="Content-type" content="text/html; charset=utf-8" />
...
<body>
<div>
    <h1>Example Domain</h1>
    <p>This domain is for use in illustrative examples in documents...</p>
</div>
</body>
</html>
```
