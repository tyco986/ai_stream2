# Vendor public keys (baked into django image)

| File | Role |
|------|------|
| `dev.pub` | Vendor age public key (Export recipient) |
| `ticket.pub` | Vendor JWT Ed25519 verify key |

Generate once (containerized; private keys stay on host volume):

```bash
bash servers/django/scripts/0_generate_vendor_keys.sh
```

- Public keys → this directory (commit to git for stable builds)
- Private keys → `secrets/vendor/` (gitignored; survive image rebuilds)
