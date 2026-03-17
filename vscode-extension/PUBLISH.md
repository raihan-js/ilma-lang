# Publishing the ILMA VS Code Extension

## Prerequisites

1. A VS Code Marketplace publisher account at https://marketplace.visualstudio.com/manage
2. Personal Access Token (PAT) from Azure DevOps with Marketplace scope
3. `vsce` tool: `npm install -g @vscode/vsce`

## First-time setup

```bash
vsce login ilmalang
# Enter your PAT when prompted
```

## Build and publish

```bash
cd vscode-extension
npm install
npm run compile
vsce package        # creates ilma-language-0.1.0.vsix
vsce publish        # publishes to Marketplace
```

## Verify publication

After publishing (takes 5-10 minutes to appear):
- https://marketplace.visualstudio.com/items?itemName=ilmalang.ilma-language

## Update the extension

1. Bump `"version"` in `package.json`
2. Add entry to `CHANGELOG.md`
3. Run `vsce publish`

## Install locally for testing

```bash
vsce package
code --install-extension ilma-language-0.1.0.vsix
```

## Extension ID

`ilmalang.ilma-language`

Users install with:
```
ext install ilmalang.ilma-language
```
