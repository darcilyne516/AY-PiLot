const { app, BrowserWindow } = require('electron');
const path = require('path');

function createWindow() {
  const win = new BrowserWindow({
    width: 900,
    height: 700,
    backgroundColor: '#000814',
    webPreferences: {
      nodeIntegration: true,
      contextIsolation: false
    }
  });

  // In a real build, we'd load the React build
  // For this project structure, we point to an index.html
  win.loadFile('index.html');
}

app.whenReady().then(createWindow);

app.on('window-all-closed', () => {
  if (process.platform !== 'darwin') app.quit();
});
