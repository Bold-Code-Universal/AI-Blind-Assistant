from flask import Flask, request, jsonify
import requests
import base64
import json
import time
import pygame
import subprocess
from gtts import gTTS

# ================================
# FFmpeg Path (USER MUST PASTE PATH)
# ================================
FFMPEG_PATH = r"Your FFmpeg Path" #like this--> C:\Users\Boboi\Downloads\ffmpeg-7.1.1-full_build\bin\ffmpeg.exe

app = Flask(__name__)

# ================================
# OpenRouter Vision Model 
# ================================
API_KEY = "Your Openrouter API Key"
MODEL = "nvidia/nemotron-nano-12b-v2-vl:free"
URL = "https://openrouter.ai/api/v1/chat/completions"

last_result = "No result yet"


# ================================
# CLEAN THE TEXT (remove Markdown)
# ================================
def clean_text(text):
    for ch in ["*", "_", "#"]:
        text = text.replace(ch, "")

    lines = [line.strip() for line in text.split("\n") if line.strip()]
    cleaned = "\n".join(lines)

    return cleaned.strip()


# ================================
# TTS: NEW FILE FOR EACH REQUEST
# ================================
def generate_and_play_tts(text):
    timestamp = int(time.time())
    mp3_file = f"tts_{timestamp}.mp3"
    wav_file = f"tts_{timestamp}.wav"

    print("🎤 Generating TTS MP3...")
    tts = gTTS(text=text, lang='en')
    tts.save(mp3_file)

    print("🔄 Converting MP3 → WAV using FFmpeg...")
    subprocess.run(
        [FFMPEG_PATH, "-y", "-i", mp3_file, wav_file],
        stdout=subprocess.PIPE,
        stderr=subprocess.PIPE
    )

    print("🔊 Playing NEW WAV file...")
    pygame.mixer.quit()
    pygame.mixer.init()
    pygame.mixer.music.load(wav_file)
    pygame.mixer.music.play()

    while pygame.mixer.music.get_busy():
        pass


# ================================
# PROCESS IMAGE FROM ESP32
# ================================
@app.route("/upload", methods=["POST"])
def process_image():
    global last_result

    data = request.get_json(force=True, silent=True)
    if not data:
        return jsonify({"error": "invalid json"}), 400

    img_b64 = data.get("image")
    if not img_b64:
        return jsonify({"error": "no image received"}), 400

    print("\n📸 Image received — sending to Vision model...")

    payload = {
        "model": MODEL,
        "messages": [
            {
                "role": "user",
                "content": [
                    {
                        "type": "text",
                        "text": "Describe the image briefly for a blind person. Read all visible text clearly and mention important objects only."
                    },
                    {
                        "type": "image_url",
                        "image_url": f"data:image/jpeg;base64,{img_b64}"
                    }
                ]
            }
        ]
    }

    headers = {
        "Authorization": f"Bearer {API_KEY}",
        "Content-Type": "application/json"
    }

    response = requests.post(URL, headers=headers, json=payload)

    if response.status_code != 200:
        print("❌ API Error:", response.text)
        return jsonify({"error": response.text}), 500

    raw_text = response.json()["choices"][0]["message"]["content"]
    cleaned = clean_text(raw_text)

    last_result = cleaned

    print("\n✔ Cleaned Vision Output:")
    print(cleaned)

    generate_and_play_tts(cleaned)

    return jsonify({"response": cleaned})


# ================================
# RETURN LAST RESULT
# ================================
@app.route("/last", methods=["GET"])
def last():
    return jsonify({"response": last_result})


# ================================
# DEFAULT ROUTE
# ================================
@app.route("/")
def home():
    return "Laptop Audio TTS Server (Gemini Vision) ACTIVE!", 200


# ================================
# START SERVER
# ================================
app.run(host="0.0.0.0", port=5000)
