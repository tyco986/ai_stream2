# DeepStream API test data

This directory stores payload templates and runtime-generated JSON files for
DeepStream API black-box tests.

- Static templates:
  - `stream_add_template.json`
  - `stream_remove_template.json`
  - `command_templates.json`
- Runtime files:
  - `cam_test_*_*.json` written by scripts under `deepstream/test/`

You can run all tests with:

```bash
python3 deepstream/test/test_all.py
```
