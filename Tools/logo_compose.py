# Compose the final logo (art render + wordmark) as SVG, then render PNG with Edge headless.
import base64, subprocess, os, sys
D = os.path.join(os.path.dirname(os.path.abspath(__file__)), "..", "Docs")
art = base64.b64encode(open(os.path.join(D, "Logo_art.png"), "rb").read()).decode()
svg = f'''<svg xmlns="http://www.w3.org/2000/svg" xmlns:xlink="http://www.w3.org/1999/xlink" viewBox="0 0 1024 1280" width="1024" height="1280">
  <title>Infinisat</title>
  <defs>
    <style>
      @font-face {{ font-family: "Rajdhani"; font-weight: 700; src: url("../Content/Slate/Fonts/Rajdhani-Bold.ttf"); }}
      @font-face {{ font-family: "Titillium Web"; font-weight: 600; src: url("../Content/Slate/Fonts/TitilliumWeb-SemiBold.ttf"); }}
    </style>
    <linearGradient id="fade" x1="0" y1="0" x2="0" y2="1">
      <stop offset="0.75" stop-color="#03060d" stop-opacity="0"/><stop offset="1" stop-color="#03060d" stop-opacity="1"/>
    </linearGradient>
    <linearGradient id="word" x1="0" y1="0" x2="1" y2="0">
      <stop offset="0" stop-color="#ffffff"/><stop offset="0.62" stop-color="#ffffff"/>
      <stop offset="0.63" stop-color="#ffb04a"/><stop offset="1" stop-color="#ffd27a"/>
    </linearGradient>
  </defs>
  <rect width="1024" height="1280" fill="#03060d"/>
  <image x="0" y="0" width="1024" height="1024" href="data:image/png;base64,{art}" xlink:href="data:image/png;base64,{art}"/>
  <rect width="1024" height="1024" fill="url(#fade)"/>
  <text x="512" y="1135" text-anchor="middle" font-family="Rajdhani, sans-serif" font-weight="700" font-size="172" letter-spacing="18" fill="url(#word)">INFINISAT</text>
  <text x="512" y="1210" text-anchor="middle" font-family="'Titillium Web', sans-serif" font-weight="600" font-size="34" letter-spacing="14" fill="#8fb4e0">FROM KOUROU TO INFINITY</text>
</svg>'''
open(os.path.join(D, "Logo.svg"), "w", encoding="utf-8").write(svg)
edge = r"C:\Program Files (x86)\Microsoft\Edge\Application\msedge.exe"
subprocess.run([edge, "--headless=new", "--disable-gpu", "--hide-scrollbars", "--allow-file-access-from-files", "--window-size=1024,1280",
                "--screenshot=" + os.path.abspath(os.path.join(D, "Logo.png")), "file:///" + os.path.abspath(os.path.join(D, "Logo.svg")).replace("\\", "/")], check=False)
print("composed")
