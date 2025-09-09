# About
The Node Code Editor plugin integrates a lightweight code editing environment directly inside Unreal Engine. It provides a dockable editor window for inspecting, modifying, and extending C++ code behind engine nodes.
The editor is designed for quick changes, inline completions, and AI-assisted coding. It does not aim to replace a full IDE (Visual Studio or Rider), but instead complements them by streamlining small modifications and enabling AI-powered explanations or function generation directly within UE.
<img width="1107" height="818" alt="image" src="https://github.com/user-attachments/assets/a5b95bd4-4ac5-40ac-90c2-20aa22aacb4b" />

# Unreal Engine AI Code Editor Plugin

An intelligent code editor plugin for Unreal Engine that integrates AI assistance directly into your development workflow.

## Quick Start Guide

### 0 - Required Pre-Steps

**0.1** Acquire an API key from **Anthropic (Claude)** or **OpenAI (ChatGPT)**

**0.2** Enable the Node Code Editor plugin in UE:
- Navigate to **Edit → Plugins** and enable *Node Code Editor*
- Restart the editor if required

### 1 - Opening the Node Code Editor

**1.0** Access the Node Code Editor through **Window → Node Code Editor** menu

**1.1** Dock the editor to a preferred location in the UE layout

### 2 - Quick Edit

**2.0** Select a node in the Blueprint or C++ editor

**2.1** Use Quick Edit to view implementation or definition directly

**2.2** Make inline changes and build immediately

### 3 - AI Features

#### 3.1 AI Code Preview
- Highlight code and press **Ctrl + Space**
- Request an explanation of the highlighted snippet

#### 3.2 AI Function Generation
- Switch to declaration view
- Leave isolated mode to see full header context
- Provide context (current line or multiple lines)
- Press **Ctrl + Space** to generate a function declaration
- Use "Generate Definition" to create the corresponding definition
- Optionally, use AI to generate the implementation

### 4 - AI Context Settings

#### 4.1 Chat Context
- **Visible Code** or **User Selection**

#### 4.2 Inline Completion Context
- **Implementation**: Entire function, Function to cursor, or N lines above
- **Declaration**: Current line or N lines above
- **Output mode**: Complete line or complete block

### 5 - Configuration

**5.1** Open **Project Settings → Node Code Editor**

**5.2** Configure:
- API key and provider
- Default model
- Inline completion defaults
- Editor preferences (font, theme, key bindings)

## Key Features

### Code Editor (Dockable Window)
- **Flexible Interface**: Dockable window that can be positioned anywhere within the Unreal Engine editor
- **Direct File Access**: Provides immediate access to node implementation and definition files
- **Seamless Development**: Edit and build code without leaving the UE environment

### AI Code Preview and Explanation
- **Source Code Display**: View the underlying source code behind engine nodes
- **AI Integration**: Send code snippets to Claude or ChatGPT APIs for detailed explanations
- **Documentation Alternative**: Understand complex built-in functions without external searches

### AI Function Generation
Generate new Blueprint-callable functions with AI assistance:

**Workflow:**
1. Switch to declaration view
2. Provide context (current line, N lines, or full function)
3. Trigger AI query using `Ctrl + Space` to generate declaration
4. Use "Generate Definition" to automatically create the function body
5. Compile directly in editor - new functions immediately become available as Blueprint nodes

### AI Context Controls

#### AI Chat Context
- **Visible Code**: Send all currently visible code to AI
- **User Selection**: Send only highlighted/selected text

#### Inline AI Completion Context
- **Implementation Options**:
  - Entire function
  - Function up to cursor position
  - N lines above cursor
- **Declaration Options**:
  - Current line
  - N lines above cursor
- **Output Options**:
  - Complete line
  - Complete block

### Additional Features
- **Search Functionality**: Find text within files
- **Navigation**: Quick "Go to line" feature
- **File Explorer Integration**: Open current file in system file explorer
- **Text Completion**: Expandable dictionary for custom completions (located in plugin Configuration folder)

## Settings and API Configuration

### Supported AI Services
- **Anthropic Claude API**
- **OpenAI ChatGPT API**

### Setup Requirements
- API key configuration in plugin settings (required)

### Customization Options
- Default AI model selection
- Inline completion behavior settings
- Font size customization
- Theme selection
- Custom key bindings

---

*The Node Code Editor is intended for lightweight editing and AI-assisted tasks.
For advanced debugging, refactoring, or project-wide development, use a dedicated IDE.
The plugin is currently in an experimental stage. Expect bugs and report issues via the support channel. https://discord.gg/FadHwQ4B
*
