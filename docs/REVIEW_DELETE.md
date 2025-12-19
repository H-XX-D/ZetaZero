# Review-delete candidates

This is a staging list of files/folders that look like local artifacts, generated outputs, or scratch items.
Nothing is deleted automatically.

## Likely safe to delete (local-only)

- `Untitled.rtf`
- `485bd120-6f2a-4ef1-a3cc-13873445c3a4_7559933.png` (random screenshot/asset)
- `llama_verbose.log`
- `semantic_trajectory.log`
- LaTeX build outputs (should be ignored):
  - `ARXIV_PAPER_DRAFT.aux`
  - `ARXIV_PAPER_DRAFT.log`
  - `ARXIV_PAPER_DRAFT.synctex(busy)`

## Needs review (don’t delete blindly)

- `Archive/` (untracked; decide whether to keep, move under `Docs/Archive/`, or remove)
- `Models/` (legacy capitalized model dir; now ignored)

## Suggested commands

Show ignored clutter only:

```sh
git status --ignored
```

Remove ignored files only (safe-ish):

```sh
git clean -fdX
```

Remove ignored + untracked (dangerous; review first):

```sh
git clean -fdx
```
