package com.aypilot.nativeapp

import android.os.Bundle
import androidx.activity.ComponentActivity
import androidx.activity.compose.setContent
import androidx.compose.foundation.background
import androidx.compose.foundation.layout.*
import androidx.compose.material3.*
import androidx.compose.runtime.*
import androidx.compose.ui.Modifier
import androidx.compose.ui.graphics.Color
import androidx.compose.ui.unit.dp
import androidx.compose.ui.text.font.FontFamily
import androidx.compose.ui.text.font.FontWeight
import androidx.compose.ui.unit.sp

class MainActivity : ComponentActivity() {
    private val engine = VoiceEngine()

    override fun onCreate(savedInstanceState: Bundle?) {
        super.onCreate(savedInstanceState)
        engine.init(48000, 512)
        
        setContent {
            AYPilotTheme {
                Surface(modifier = Modifier.fillMaxSize(), color = Color(0xFF000814)) {
                    VoiceChangerUI(engine)
                }
            }
        }
    }
}

@Composable
fun VoiceChangerUI(engine: VoiceEngine) {
    var selectedPreset by remember { mutableStateOf(0) }
    val presets = listOf("Natural", "Female", "Male", "Deep", "High", "Robot", "Whisper", "AI Clone")
    val electricViolet = Color(0xFF8A2BE2)

    Column(modifier = Modifier.padding(24.dp)) {
        Text(
            text = "AY PILOT",
            color = electricViolet,
            fontSize = 32.sp,
            fontWeight = FontWeight.Bold,
            fontFamily = FontFamily.SansSerif
        )
        
        Spacer(modifier = Modifier.height(32.dp))
        
        Text("Voice Presets", color = Color.White, fontSize = 18.sp)
        
        Spacer(modifier = Modifier.height(16.dp))
        
        presets.forEachIndexed { index, name ->
            Button(
                onClick = { 
                    selectedPreset = index
                    engine.setPreset(index)
                },
                modifier = Modifier.fillMaxWidth().padding(vertical = 4.dp),
                colors = ButtonDefaults.buttonColors(
                    containerColor = if (selectedPreset == index) electricViolet else Color(0xFF1A1A2E)
                )
            ) {
                Text(name, color = Color.White)
            }
        }
        
        Spacer(modifier = Modifier.weight(1f))
        
        Row(modifier = Modifier.fillMaxWidth(), horizontalArrangement = Arrangement.SpaceBetween) {
            Text("Latency: 35ms", color = Color(0xFF00FF41))
            Text("CPU: 12%", color = Color(0xFF00FF41))
        }
    }
}

@Composable
fun AYPilotTheme(content: @Composable () -> Unit) {
    MaterialTheme(content = content)
}
