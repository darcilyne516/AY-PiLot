import React, { useState } from 'react';

const App = () => {
  const [selectedPreset, setSelectedPreset] = useState(0);
  const presets = ["Natural", "Female", "Male", "Deep", "High", "Robot", "Whisper", "AI Clone"];

  const styles = {
    container: {
      backgroundColor: '#000814',
      color: 'white',
      height: '100vh',
      padding: '40px',
      fontFamily: 'Inter, sans-serif',
      display: 'flex',
      flexDirection: 'column'
    },
    header: {
      color: 'oklch(0.62 0.28 295)', // Electric Violet
      fontSize: '36px',
      fontWeight: 'bold',
      fontFamily: 'Space Grotesk, sans-serif',
      marginBottom: '40px'
    },
    panel: {
      background: 'rgba(255, 255, 255, 0.05)',
      backdropFilter: 'blur(10px)',
      borderRadius: '16px',
      padding: '24px',
      border: '1px solid rgba(255, 255, 255, 0.1)'
    },
    button: (active) => ({
      backgroundColor: active ? 'oklch(0.62 0.28 295)' : '#1A1A2E',
      color: 'white',
      border: 'none',
      borderRadius: '8px',
      padding: '12px 24px',
      margin: '8px',
      cursor: 'pointer',
      transition: 'all 0.2s ease',
      fontSize: '16px'
    }),
    footer: {
      marginTop: 'auto',
      display: 'flex',
      justifyContent: 'space-between',
      color: '#00FF41', // Phosphor Green
      fontFamily: 'JetBrains Mono, monospace',
      fontSize: '14px'
    }
  };

  return (
    <div style={styles.container}>
      <div style={styles.header}>AY PILOT</div>
      
      <div style={styles.panel}>
        <h3 style={{ marginBottom: '20px' }}>Voice Presets</h3>
        <div style={{ display: 'flex', flexWrap: 'wrap' }}>
          {presets.map((name, index) => (
            <button
              key={name}
              style={styles.button(selectedPreset === index)}
              onClick={() => setSelectedPreset(index)}
            >
              {name}
            </button>
          ))}
        </div>
      </div>

      <div style={styles.footer}>
        <span>LATENCY: 38ms</span>
        <span>CPU: 18%</span>
        <span>STATUS: ACTIVE</span>
      </div>
    </div>
  );
};

export default App;
