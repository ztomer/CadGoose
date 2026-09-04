#!/usr/bin/env python3
"""Prompt builders + output scorers for eval_goose_ai.py.

Split from the evaluator (house 500-line cap); nothing here talks to the
network — pure text in, scores/prompts out.
"""

import random
import re

# Emoji the scorers look for in *model output*. They are data, not decoration, so they are
# written as codepoint escapes — tools/house_gates/check_no_emoji.py policies literal
# emoji glyphs, and
# these strings must keep matching what the model actually emits.
DUCK = "\U0001F986"    # duck
GOOSE = "\U0001FABF"   # goose
SKULL = "\U0001F480"   # skull
IMP = "\U0001F608"     # smiling face with horns

# ──────────────────────────────────────────────────────────────
# Prompt builder (mirrors ai_text_meme.mm BuildPrompt())
# ──────────────────────────────────────────────────────────────

EVIL_PERSONALITIES = {
    0.0: "a friendly, harmless goose who loves everyone",
    0.2: "a slightly mischievous goose with a playful streak",
    0.4: "a moderately evil goose who enjoys minor chaos",
    0.6: "a quite evil goose who delights in causing trouble",
    0.8: "a very evil goose with a dark sense of humor and no remorse",
    1.0: "the most evil goose in existence — pure chaos incarnate",
}

BEHAVIOR_LISTS = {
    "minimal": ["ball", "honcker"],
    "fun": ["ball", "breadCrumbs", "hats", "rainbow"],
    "chaos": ["ball", "breadCrumbs", "hats", "rainbow", "acid", "anger", "autumnLeaves", "honcker", "jail", "portals"],
    "all": ["ball", "breadCrumbs", "hats", "rainbow", "acid", "anger", "autumnLeaves", "honcker", "jail", "portals", "drag", "nametag", "health", "pomodoro"],
}

COLOR_MODES = ["light", "dark", "system"]


def build_text_meme_prompt(
    evil_level: float = 0.4,
    behaviors: str = "fun",
    color_mode: str = "light",
    seed: Optional[int] = None,
) -> str:
    """Build the exact prompt the goose uses for text meme generation."""
    if seed is None:
        seed = (random.randint(0, 32767) << 16) ^ random.randint(0, 32767)

    personality = EVIL_PERSONALITIES.get(evil_level, f"a goose with evil level {evil_level}")
    behavior_str = ", ".join(BEHAVIOR_LISTS.get(behaviors, behaviors.split(",")))

    return (
        f"You are {personality}. "
        f"Generate ONE short, funny text message that a goose like you would leave behind. "
        f"Current active behaviors: {behavior_str}. "
        f"Color theme: {color_mode}. "
        f"Random seed: {seed}. "
        f"Be creative and absurd. Output ONLY the message text, nothing else. No quotes. Max 120 characters."
    )


def build_chat_prompt(message: str, goose_name: str = "Gandalf") -> list[dict]:
    """Build the chat message list the goose AI chat uses."""
    return [
        {
            "role": "system",
            "content": (
                f"You are {goose_name}, a chaotic and hilarious goose. "
                f"Respond in character as a goose. Use HONK occasionally. "
                f"Be funny, absurd, and slightly unhinged. "
                f"Keep responses under 200 characters."
            ),
        },
        {"role": "user", "content": message},
    ]


# ──────────────────────────────────────────────────────────────
# Scoring validators
# ──────────────────────────────────────────────────────────────


def score_text_meme(text: str, evil_level: float = 0.4) -> dict:
    """Score a text meme on multiple quality dimensions. Returns dict with scores 0-100."""
    scores = {}
    reasons = []

    stripped = text.strip().strip("'\"").strip()

    # ── Format compliance (0-100) ──
    format_score = 100
    if not stripped:
        format_score = 0
        reasons.append("empty response")
    else:
        if len(stripped) > 120:
            format_score -= 30
            reasons.append(f"too long ({len(stripped)} > 120)")
        if stripped.startswith('"') or stripped.endswith('"'):
            format_score -= 15
            reasons.append("has quotes")
        if any(
            m in stripped.lower()
            for m in ["here is", "here's", "i would", "i'll", "sure,", "okay,", "of course", "let me"]
        ):
            format_score -= 25
            reasons.append("conversational filler")
        if stripped.lower().startswith(("the goose", "a goose", "this goose")):
            format_score -= 10
            reasons.append("third-person description instead of first-person message")
    scores["format"] = max(0, format_score)

    # ── Brevity (0-100) ──
    # Ideal: 15-80 chars. Short enough to be a sticky note, long enough to be funny.
    length = len(stripped)
    if 15 <= length <= 80:
        brevity = 100
    elif length < 15:
        brevity = max(20, length * 5)
    elif length <= 120:
        brevity = max(50, 100 - (length - 80))
    else:
        brevity = max(0, 100 - (length - 120))
    scores["brevity"] = brevity

    # ── Goose persona (0-100) ──
    # Does it feel like a goose wrote it?
    goose_signals = [
        "honk", "goose", "geese", "honk!", DUCK, GOOSE,
        "bread", "crumb", "pond", "waddle", "feather", "beak",
        "hiss", "flap", "nest", "egg", "swan", "duck",
    ]
    lower = stripped.lower()
    goose_hits = sum(1 for s in goose_signals if s in lower)
    if goose_hits >= 2:
        goose_score = 100
    elif goose_hits == 1:
        goose_score = 75
    elif any(c in lower for c in ["chaos", "evil", "mischief", "destroy", "wrath", "revenge"]):
        goose_score = 60  # evil goose vibes without explicit goose words
    else:
        goose_score = 20
        reasons.append("no goose persona signals")
    scores["goose_persona"] = goose_score

    # ── Humor / creativity (0-100) ──
    # Heuristic: absurdity, unexpectedness, wordplay
    humor_score = 50  # baseline

    # Positive signals
    if any(c in stripped for c in ["!", DUCK, GOOSE, SKULL, IMP]):
        humor_score += 10
    if len(stripped.split()) >= 3:  # not just "honk"
        humor_score += 5
    if any(
        c in lower
        for c in ["steal", "attack", "destroy", "chaos", "evil", "wrath", "doom",
                   "hiss", "menace", "terror", "supreme", "overlord", "dictator"]
    ):
        humor_score += 15  # goose grandiosity is funny
    if any(c in lower for c in ["bread", "crumb", "pond", "waddle", "feather"]):
        humor_score += 10  # goose life references
    if re.search(r"[a-z]+\s+\d+\s+[a-z]+", lower):  # e.g. "3 breads stolen"
        humor_score += 5  # specificity is funny
    if re.search(r"\b(i|my|me)\b", lower) and re.search(r"\b(will|shall|gonna|must|should)\b", lower):
        humor_score += 5  # declarative goose intent

    # Negative signals
    if lower.startswith(("i am", "i'm a", "i am a", "this is")):
        humor_score -= 10  # boring intro
    if re.search(r"please|thank you|sorry|apologize", lower):
        humor_score -= 15  # too polite for a goose
    if len(set(stripped.lower())) < 8:
        humor_score -= 20  # very repetitive

    scores["humor"] = max(0, min(100, humor_score))

    # ── Evil level match (0-100) ──
    # Does the tone match the requested evil level?
    evil_words_positive = ["chaos", "evil", "destroy", "attack", "wrath", "doom", "menace",
                           "terror", "supreme", "overlord", "dictator", "vengeance", "reign",
                           "fear", "dark", "sinister", "malice", "hatred", "curse"]
    evil_words_negative = ["love", "friend", "happy", "kind", "sweet", "nice", "gentle",
                           "peace", "hug", "care", "wonderful", "beautiful"]

    evil_pos = sum(1 for w in evil_words_positive if w in lower)
    evil_neg = sum(1 for w in evil_words_negative if w in lower)

    if evil_level >= 0.6:
        # Should be evil
        if evil_pos >= 2:
            evil_match = 100
        elif evil_pos >= 1:
            evil_match = 75
        elif evil_neg >= 1:
            evil_match = 20
            reasons.append("too nice for high evil level")
        else:
            evil_match = 50
    elif evil_level <= 0.2:
        # Should be friendly
        if evil_neg >= 1:
            evil_match = 100
        elif evil_pos >= 1:
            evil_match = 40
            reasons.append("too aggressive for low evil level")
        else:
            evil_match = 80
    else:
        # Mid range — either is fine
        evil_match = 80

    scores["evil_match"] = evil_match

    # ── Overall weighted score ──
    weights = {"format": 0.20, "brevity": 0.15, "goose_persona": 0.25, "humor": 0.25, "evil_match": 0.15}
    overall = sum(scores[k] * weights[k] for k in weights)

    return {
        "overall": round(overall),
        "dimensions": scores,
        "reasons": reasons,
        "text": stripped,
    }


def score_chat_response(text: str, user_message: str) -> dict:
    """Score a chat response on quality dimensions."""
    scores = {}
    reasons = []

    stripped = text.strip()
    lower = stripped.lower()

    # ── Format (0-100) ──
    format_score = 100
    if not stripped:
        format_score = 0
        reasons.append("empty response")
    if len(stripped) > 300:
        format_score -= 20
        reasons.append(f"too long ({len(stripped)} chars)")
    if any(m in lower for m in ["as an ai", "as a language model", "i cannot", "i'm sorry but"]):
        format_score -= 40
        reasons.append("AI refusal pattern")
    scores["format"] = max(0, format_score)

    # ── Goose persona (0-100) ──
    goose_signals = ["honk", "goose", DUCK, GOOSE, "hiss", "waddle", "feather", "beak"]
    goose_hits = sum(1 for s in goose_signals if s in lower)
    if goose_hits >= 2:
        goose_score = 100
    elif goose_hits == 1:
        goose_score = 75
    else:
        goose_score = 30
        reasons.append("no goose persona")
    scores["goose_persona"] = goose_score

    # ── Relevance (0-100) ──
    # Does the response address the user's message?
    user_words = set(user_message.lower().split())
    response_words = set(lower.split())
    overlap = user_words & response_words
    # Remove common words from overlap
    stop = {"the", "a", "an", "is", "are", "to", "of", "and", "or", "but", "in", "on", "at", "it", "i", "you", "my", "your"}
    meaningful_overlap = overlap - stop
    if len(meaningful_overlap) >= 2:
        relevance = 100
    elif len(meaningful_overlap) == 1:
        relevance = 70
    else:
        relevance = 40
        reasons.append("no topical overlap with user message")
    scores["relevance"] = relevance

    # ── Humor (0-100) ──
    humor_score = 50
    if any(c in stripped for c in ["!", DUCK, GOOSE, SKULL]):
        humor_score += 10
    if any(c in lower for c in ["honk", "chaos", "evil", "mischief", "steal", "attack"]):
        humor_score += 15
    if re.search(r"\b(i|my|me)\b", lower) and re.search(r"\b(will|shall|gonna|must)\b", lower):
        humor_score += 5
    if re.search(r"please|thank you|sorry|apologize|cannot|unable", lower):
        humor_score -= 20
    scores["humor"] = max(0, min(100, humor_score))

    # ── Overall ──
    weights = {"format": 0.20, "goose_persona": 0.30, "relevance": 0.25, "humor": 0.25}
    overall = sum(scores[k] * weights[k] for k in weights)

    return {
        "overall": round(overall),
        "dimensions": scores,
        "reasons": reasons,
        "text": stripped,
    }


