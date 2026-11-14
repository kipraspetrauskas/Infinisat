import os, re
os.chdir(r"C:\Dev\Infinisat\Source\Infinisat")
s = open("SISGameUI.cpp", encoding="utf-8").read()

# 1) project fonts (OFL) with a safe fallback to the engine font
font_helper = r'''
	// Titillium Web (UI text), Rajdhani (headings), Share Tech Mono (readouts) - SIL Open Font License
	FSlateFontInfo ISFont(const char* Style, int32 Size)
	{
		const FString S(Style);
		FString File = TEXT("TitilliumWeb-Regular.ttf");
		float Scale = 1.08f;
		if (S == TEXT("Bold")) { File = TEXT("Rajdhani-Bold.ttf"); Scale = 1.2f; }
		else if (S == TEXT("SemiBold")) { File = TEXT("TitilliumWeb-SemiBold.ttf"); }
		else if (S == TEXT("Mono")) { File = TEXT("ShareTechMono-Regular.ttf"); Scale = 1.f; }
		const FString Path = FPaths::ProjectContentDir() / TEXT("Slate/Fonts") / File;
		if (!FPaths::FileExists(Path)) return FCoreStyle::GetDefaultFontStyle(Style, Size);
		return FSlateFontInfo(Path, FMath::RoundToInt(Size * Scale));
	}
'''
anchor = "	FButtonStyle MakeStyle(const FLinearColor& N, const FLinearColor& H, const FLinearColor& P)"
assert anchor in s
s = s.replace(anchor, font_helper + "\n" + anchor, 1)
s = s.replace("FCoreStyle::GetDefaultFontStyle(", "ISFont(")
# keep the helper's own fallback call intact
s = s.replace("if (!FPaths::FileExists(Path)) return ISFont(Style, Size);", "if (!FPaths::FileExists(Path)) return FCoreStyle::GetDefaultFontStyle(Style, Size);")

# 2) palette: deep glass panels, cleaner accents
rep = {
    "const FLinearColor CText(0.92f, 0.95f, 1.f);": "const FLinearColor CText(0.93f, 0.96f, 1.f);",
    "const FLinearColor CDim(0.62f, 0.7f, 0.82f);": "const FLinearColor CDim(0.6f, 0.69f, 0.8f);",
    "const FLinearColor CAccent(0.35f, 0.82f, 1.f);": "const FLinearColor CAccent(0.36f, 0.86f, 1.f);",
    "const FLinearColor CPanel(0.015f, 0.04f, 0.1f, 0.9f);": "const FLinearColor CPanel(0.012f, 0.022f, 0.05f, 0.78f);",
    "const FLinearColor CPanel2(0.04f, 0.09f, 0.2f, 0.95f);": "const FLinearColor CPanel2(0.025f, 0.06f, 0.13f, 0.86f);",
}
for a, b in rep.items():
    assert a in s, a
    s = s.replace(a, b)

# 3) buttons: rounded, thin outline, clear hover/pressed states
a = """	FButtonStyle MakeStyle(const FLinearColor& N, const FLinearColor& H, const FLinearColor& P)
	{
		FButtonStyle S;
		S.SetNormal(FSlateRoundedBoxBrush(N, 6.f));
		S.SetHovered(FSlateRoundedBoxBrush(H, 6.f));
		S.SetPressed(FSlateRoundedBoxBrush(P, 6.f));
		S.SetDisabled(FSlateRoundedBoxBrush(FLinearColor(0.1f, 0.1f, 0.12f, 0.8f), 6.f));
		S.SetNormalPadding(FMargin(10, 6));
		S.SetPressedPadding(FMargin(10, 7, 10, 5));
		return S;
	}"""
b = """	FButtonStyle MakeStyle(const FLinearColor& N, const FLinearColor& H, const FLinearColor& P)
	{
		FButtonStyle S;
		const FLinearColor Edge(0.4f, 0.8f, 1.f, 0.28f);
		S.SetNormal(FSlateRoundedBoxBrush(N, 8.f, Edge, 1.f));
		S.SetHovered(FSlateRoundedBoxBrush(H, 8.f, FLinearColor(0.55f, 0.9f, 1.f, 0.7f), 1.f));
		S.SetPressed(FSlateRoundedBoxBrush(P, 8.f, Edge, 1.f));
		S.SetDisabled(FSlateRoundedBoxBrush(FLinearColor(0.06f, 0.07f, 0.09f, 0.7f), 8.f));
		S.SetNormalPadding(FMargin(14, 8));
		S.SetPressedPadding(FMargin(14, 9, 14, 7));
		return S;
	}"""
assert a in s
s = s.replace(a, b)
s = s.replace("StyleNormal = MakeStyle(FLinearColor(0.08f, 0.16f, 0.3f), FLinearColor(0.12f, 0.26f, 0.45f), FLinearColor(0.05f, 0.1f, 0.2f));",
              "StyleNormal = MakeStyle(FLinearColor(0.04f, 0.08f, 0.15f, 0.92f), FLinearColor(0.07f, 0.16f, 0.28f, 0.95f), FLinearColor(0.02f, 0.05f, 0.1f));")
s = s.replace("StylePrimary = MakeStyle(FLinearColor(0.0f, 0.45f, 0.7f), FLinearColor(0.05f, 0.6f, 0.9f), FLinearColor(0.0f, 0.3f, 0.5f));",
              "StylePrimary = MakeStyle(FLinearColor(0.0f, 0.42f, 0.66f), FLinearColor(0.02f, 0.58f, 0.86f), FLinearColor(0.0f, 0.3f, 0.48f));")
s = s.replace("StyleSelected = MakeStyle(FLinearColor(0.75f, 0.45f, 0.05f), FLinearColor(0.9f, 0.55f, 0.1f), FLinearColor(0.6f, 0.35f, 0.05f));",
              "StyleSelected = MakeStyle(FLinearColor(0.78f, 0.46f, 0.08f), FLinearColor(0.92f, 0.58f, 0.14f), FLinearColor(0.6f, 0.35f, 0.05f));")
open("SISGameUI.cpp", "w", encoding="utf-8").write(s)

h = open("SISGameUI.h", encoding="utf-8").read()
h = h.replace("FSlateRoundedBoxBrush PanelBrush = FSlateRoundedBoxBrush(FLinearColor::White, 10.f);",
              "FSlateRoundedBoxBrush PanelBrush = FSlateRoundedBoxBrush(FLinearColor::White, 14.f, FLinearColor(0.45f, 0.82f, 1.f, 0.35f), 1.f);")
open("SISGameUI.h", "w", encoding="utf-8").write(h)

g = open(r"C:\Dev\Infinisat\Config\DefaultGame.ini", encoding="utf-8").read()
if "Slate/Fonts" not in g:
    g = g.replace('+DirectoriesToAlwaysCook=(Path="/Engine/BasicShapes")', '+DirectoriesToAlwaysCook=(Path="/Engine/BasicShapes")\n+DirectoriesToAlwaysStageAsNonUFS=(Path="Slate/Fonts")')
    open(r"C:\Dev\Infinisat\Config\DefaultGame.ini", "w", encoding="utf-8").write(g)
print("ui style patched")
