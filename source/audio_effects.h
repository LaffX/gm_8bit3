#pragma once
#include <cassert>
#include <cstdint>
#include <cstring>
#include <cmath>

namespace AudioEffects {

    // Идентификаторы эффектов (старые и новые)
    enum {
        EFF_NONE,           // 0
        EFF_BITCRUSH,       // 1
        EFF_DESAMPLE,       // 2
        // Новые пресеты (фиксированные)
        EFF_COMB,           // 3
        EFF_DARTHVADER,     // 4
        EFF_RADIO,          // 5
        EFF_ROBOT,          // 6
        EFF_ALIEN,          // 7
        EFF_OVERDRIVE,      // 8
        EFF_DISTORTION,     // 9
        EFF_TELEPHONE,      // 10
        EFF_MEGAPHONE,      // 11
        EFF_CHIPMUNK,       // 12
        EFF_SLOWMOTION,     // 13
        // Комбинированный пресет (новый)
        EFF_COMBO = 14      // 14 - Comb + Robot + Darth Vader
    };

    // Вспомогательная функция клиппинга
    static inline int16_t Clamp16(int32_t sample) {
        if (sample > 32767) return 32767;
        if (sample < -32768) return -32768;
        return (int16_t)sample;
    }



    // Чистильщик
    void BitCrush(uint16_t* sampleBuffer, int samples) {
        int16_t* audio = reinterpret_cast<int16_t*>(sampleBuffer);
        const float sampleRate = 22050.0f;

        // 1. Основной дисторшн
        for (int i = 0; i < samples; ++i) {
            float x = audio[i] / 32768.0f;
            x *= 4.2f;
            if (x > 1.0f) x = 1.0f;
            else if (x < -1.0f) x = -1.0f;
            audio[i] = (int16_t)(x * 32767.0f);
        }

        // 2. High-pass
        const float hpAlpha = 1.0f - expf(-2.0f * M_PI * 450.0f / sampleRate);
        float hpPrevIn = 0.0f;
        float hpOut = 0.0f;

        for (int i = 0; i < samples; ++i) {
            float x = (float)audio[i];
            hpOut = (1.0f - hpAlpha) * (hpOut + x - hpPrevIn);
            hpPrevIn = x;
            audio[i] = Clamp16((int32_t)hpOut);
        }

        // 3. Low-pass
        const float lpAlpha = 1.0f - expf(-2.0f * M_PI * 2800.0f / sampleRate);
        float lpOut = 0.0f;

        for (int i = 0; i < samples; ++i) {
            lpOut += lpAlpha * ((float)audio[i] - lpOut);
            audio[i] = Clamp16((int32_t)lpOut);
        }

        // 4. Доп. жёсткость
        const int32_t clipLevel = 14000;
        for (int i = 0; i < samples; ++i) {
            int32_t s = (int32_t)(audio[i] * 1.5f);
            if (s > clipLevel) s = clipLevel;
            else if (s < -clipLevel) s = -clipLevel;
            audio[i] = Clamp16(s);
        }
    }


    // рация
    void Desample(uint16_t* sampleBuffer, int samples) {
        int16_t* audio = reinterpret_cast<int16_t*>(sampleBuffer);

        const float sampleRate = 22050.0f;

        // --- 1. High-pass (жёстко режем низ) ---
        const float hpAlpha = 1.0f - expf(-2.0f * M_PI * 800.0f / sampleRate);
        float prev = 0, out = 0;

        for (int i = 0; i < samples; ++i) {
            out = (1.0f - hpAlpha) * (out + audio[i] - prev);
            prev = audio[i];
            audio[i] = Clamp16((int32_t)out);
        }

        // --- 2. Low-pass (чуть режем верх) ---
        const float lpAlpha = 1.0f - expf(-2.0f * M_PI * 3500.0f / sampleRate);
        float lp = 0;

        for (int i = 0; i < samples; ++i) {
            lp += lpAlpha * (audio[i] - lp);
            audio[i] = Clamp16((int32_t)lp);
        }

        // --- 3. Лёгкий биткраш ---
        const int quant = 300;
        for (int i = 0; i < samples; ++i) {
            int32_t s = audio[i];
            s = (s / quant) * quant;
            audio[i] = Clamp16(s);
        }
    }

    // -------------------- Новые фиксированные пресеты --------------------

    static float s_ringPhase = 0.0f;
    static float s_hpPrev = 0.0f;
    static float s_lpPrev = 0.0f;

    void ApplyComb(uint16_t* sampleBuffer, int samples) {
        int16_t* audio = reinterpret_cast<int16_t*>(sampleBuffer);

        const float sampleRate = 22050.0f;
        const float wetMix = 0.60f;
        const float dryMix = 1.0f - wetMix;

        // --- 1. Pitch shift DOWN ---
        // 0.75 = на ~4 полутона ниже. Читаем сжатый диапазон входа → растягиваем → ниже тон.
        // Последние ~25% буфера "теряются" — на коротких чанках незаметно.
        const float pitchRatio = 0.75f;
        {
            // Копируем оригинал во временный буфер
            static int16_t tmpBuf[16384];
            int copyLen = samples < 16384 ? samples : 16384;
            memcpy(tmpBuf, audio, copyLen * sizeof(int16_t));

            for (int i = 0; i < copyLen; ++i) {
                float srcPos = i * pitchRatio;
                int   idx0 = (int)srcPos;
                int   idx1 = (idx0 + 1 < copyLen) ? idx0 + 1 : idx0;
                float frac = srcPos - idx0;
                audio[i] = (int16_t)((1.0f - frac) * tmpBuf[idx0] + frac * tmpBuf[idx1]);
            }
        }

        // --- 2. Кольцевая модуляция (оставляем, но ещё ниже несущая) ---
        // 150 Гц — добавляет субгармоники, звук становится "грудным"
        const float ringFreq = 150.0f;
        const float ringStep = (2.0f * M_PI * ringFreq) / sampleRate;
        const float ringDepth = 0.55f;

        for (int i = 0; i < samples; ++i) {
            float mod = 1.0f - ringDepth + ringDepth * sinf(s_ringPhase);
            s_ringPhase += ringStep;
            if (s_ringPhase > 2.0f * M_PI) s_ringPhase -= 2.0f * M_PI;

            int32_t dry = audio[i];
            int32_t wet = (int32_t)(audio[i] * mod);
            audio[i] = Clamp16((int32_t)(dry * dryMix + wet * wetMix));
        }

        // --- 3. Клиппирование — оставляем мягким, иначе каша ---
        const int32_t clipLevel = 13000;
        const float   driveGain = 1.8f;
        for (int i = 0; i < samples; ++i) {
            int32_t s = (int32_t)(audio[i] * driveGain);
            s = s > clipLevel ? clipLevel : (s < -clipLevel ? -clipLevel : s);
            audio[i] = Clamp16(s);
        }

        // --- 4. Квантование ---
        const int quant = 160;
        for (int i = 0; i < samples; ++i) {
            int32_t s = audio[i];
            s = (s / quant) * quant;
            audio[i] = Clamp16(s);
        }

        // --- 5. Гребенчатый фильтр ---
        const int   combDelay = 50;
        const float combMix = 0.28f;
        for (int i = combDelay; i < samples; ++i) {
            int32_t s = (int32_t)(audio[i] + audio[i - combDelay] * combMix);
            audio[i] = Clamp16(s);
        }

        // --- 6. Полосовой фильтр ---
        // HP 250 Гц — убираем совсем низкий гул
        const float hpAlpha = 1.0f - expf(-2.0f * M_PI * 250.0f / sampleRate);
        float hpPrev = s_hpPrev, hpOut = 0.0f;
        for (int i = 0; i < samples; ++i) {
            hpOut = (1.0f - hpAlpha) * (hpOut + audio[i] - hpPrev);
            hpPrev = audio[i];
            audio[i] = Clamp16((int32_t)hpOut);
        }
        s_hpPrev = hpPrev;

        // LP 2500 Гц — срезаем верхушку агрессивнее, голос "темнее"
        const float lpAlpha = 1.0f - expf(-2.0f * M_PI * 2500.0f / sampleRate);
        float lpPrev = s_lpPrev;
        for (int i = 0; i < samples; ++i) {
            lpPrev += lpAlpha * (audio[i] - lpPrev);
            audio[i] = Clamp16((int32_t)lpPrev);
        }
        s_lpPrev = lpPrev;
    }

    void ApplyDarthVader(uint16_t* sampleBuffer, int samples) {
        int16_t* audio = reinterpret_cast<int16_t*>(sampleBuffer);
        // понижение тона (пропускаем каждый второй сэмпл) + легкая модуляция
        for (int i = 1; i < samples; ++i) {
            int32_t s = audio[i] + audio[i - 1];
            s = s / 2; // усреднение для сглаживания
            // легкое искажение
            s = s * 1.2;
            audio[i] = Clamp16(s);
        }
        // добавляем низкочастотный гул (простой интегратор)
        int32_t acc = 0;
        for (int i = 0; i < samples; ++i) {
            acc = acc + audio[i];
            audio[i] = Clamp16(acc);
        }
    }

    // робот
    void ApplyRadio(uint16_t* sampleBuffer, int samples) {
        int16_t* audio = reinterpret_cast<int16_t*>(sampleBuffer);

        const int delay = 30;
        const float feedback = 0.6f;

        for (int i = delay; i < samples; ++i) {
            int32_t delayed = audio[i - delay];
            int32_t s = audio[i] + delayed * feedback;
            audio[i] = Clamp16(s);
        }

        // немного квантования для "цифровости"
        const int quant = 400;
        for (int i = 0; i < samples; ++i) {
            int32_t s = audio[i];
            s = (s / quant) * quant;
            audio[i] = Clamp16(s);
        }
    }




// Демон
    void ApplyRobot(uint16_t* sampleBuffer, int samples) {
        int16_t* audio = reinterpret_cast<int16_t*>(sampleBuffer);

        static int16_t temp[16384];
        int len = samples < 16384 ? samples : 16384;
        memcpy(temp, audio, len * sizeof(int16_t));

        // --- 1. Pitch down ---
        const float pitch = 0.65f;

        for (int i = 0; i < len; ++i) {
            float src = i * pitch;
            int i0 = (int)src;
            int i1 = (i0 + 1 < len) ? i0 + 1 : i0;
            float t = src - i0;

            audio[i] = (int16_t)((1 - t) * temp[i0] + t * temp[i1]);
        }

        // --- 2. Второй слой (двоение) ---
        for (int i = 20; i < len; ++i) {
            int32_t s = audio[i] + temp[i - 20] * 0.5f;
            audio[i] = Clamp16(s);
        }

        // --- 3. Лёгкий дисторшн ---
        for (int i = 0; i < len; ++i) {
            float s = audio[i] / 32768.0f;
            s *= 1.8f;
            if (s > 1.0f) s = 1.0f;
            else if (s < -1.0f) s = -1.0f;
            audio[i] = (int16_t)(s * 32767);
        }
    }

    // прищелец
    void ApplyAlien(uint16_t* sampleBuffer, int samples) {
        int16_t* audio = reinterpret_cast<int16_t*>(sampleBuffer);

        static int16_t temp[16384];
        int len = samples < 16384 ? samples : 16384;
        memcpy(temp, audio, len * sizeof(int16_t));

        const float pitchRatio = 1.35f; // выше тон
        static float vibPhase = 0.0f;
        const float sampleRate = 22050.0f;
        const float vibFreq = 11.0f; // быстрое дрожание
        const float vibStep = (2.0f * M_PI * vibFreq) / sampleRate;

        for (int i = 0; i < len; ++i) {
            float vibrato = sinf(vibPhase) * 3.0f; // 3 сэмпла модуляции
            vibPhase += vibStep;
            if (vibPhase > 2.0f * M_PI) vibPhase -= 2.0f * M_PI;

            float src = i * pitchRatio + vibrato;
            if (src >= len - 1) src = (float)(len - 1);
            if (src < 0) src = 0;

            int i0 = (int)src;
            int i1 = (i0 + 1 < len) ? i0 + 1 : i0;
            float t = src - i0;

            float mainVoice = (1.0f - t) * temp[i0] + t * temp[i1];

            // Лёгкое двоение
            int d = 9;
            float doubled = (i >= d) ? temp[i - d] * 0.35f : 0.0f;

            audio[i] = Clamp16((int32_t)(mainVoice + doubled));
        }

        // Немного подчистим низ, чтобы голос был тоньше
        float prevIn = 0.0f, hpOut = 0.0f;
        const float hpAlpha = 1.0f - expf(-2.0f * M_PI * 700.0f / sampleRate);

        for (int i = 0; i < len; ++i) {
            float x = (float)audio[i];
            hpOut = (1.0f - hpAlpha) * (hpOut + x - prevIn);
            prevIn = x;
            audio[i] = Clamp16((int32_t)hpOut);
        }
    }

    // овердрайв
    void ApplyOverdrive(uint16_t* sampleBuffer, int samples) {
        int16_t* audio = reinterpret_cast<int16_t*>(sampleBuffer);

        for (int i = 0; i < samples; ++i) {
            float x = audio[i] / 32768.0f;

            // Жёсткий предгейн
            x *= 5.5f;

            // Более злой перегруз через tanh
            float y = tanhf(x * 1.8f);

            // Ещё немного поджимаем, чтобы был плотнее
            y *= 1.15f;
            if (y > 1.0f) y = 1.0f;
            else if (y < -1.0f) y = -1.0f;

            audio[i] = (int16_t)(y * 32767.0f);
        }

        // Добавим лёгкий "грязный хвост" короткой задержкой
        const int delay = 12;
        for (int i = delay; i < samples; ++i) {
            int32_t s = audio[i] + audio[i - delay] / 4;
            audio[i] = Clamp16(s);
        }
    }

    void ApplyDistortion(uint16_t* sampleBuffer, int samples) {
        int16_t* audio = reinterpret_cast<int16_t*>(sampleBuffer);
        for (int i = 0; i < samples; ++i) {
            float s = audio[i] / 32768.0f;
            s = s * 5.0f;
            if (s > 1.0f) s = 1.0f;
            else if (s < -1.0f) s = -1.0f;
            audio[i] = (int16_t)(s * 32767);
        }
    }

    void ApplyTelephone(uint16_t* sampleBuffer, int samples) {
        int16_t* audio = reinterpret_cast<int16_t*>(sampleBuffer);
        const float sampleRate = 22050.0f;

        // 1. Срезаем низ и часть верха
        const float hpAlpha = 1.0f - expf(-2.0f * M_PI * 500.0f / sampleRate);
        float hpPrevIn = 0.0f, hpOut = 0.0f;

        for (int i = 0; i < samples; ++i) {
            float x = (float)audio[i];
            hpOut = (1.0f - hpAlpha) * (hpOut + x - hpPrevIn);
            hpPrevIn = x;
            audio[i] = Clamp16((int32_t)hpOut);
        }

        const float lpAlpha = 1.0f - expf(-2.0f * M_PI * 3200.0f / sampleRate);
        float lpOut = 0.0f;

        for (int i = 0; i < samples; ++i) {
            lpOut += lpAlpha * ((float)audio[i] - lpOut);
            audio[i] = Clamp16((int32_t)lpOut);
        }

        // 2. Биткраш
        const int quant = 420;
        for (int i = 0; i < samples; ++i) {
            int32_t s = audio[i];
            s = (s / quant) * quant;
            audio[i] = Clamp16(s);
        }

        // 3. Редкие "срывы" сигнала
        for (int i = 0; i < samples; ++i) {
            if ((i % 97) < 4) {
                audio[i] = audio[i] / 5;
            }
            if ((i % 211) == 0) {
                audio[i] = -audio[i];
            }
        }

        // 4. Лёгкое цифровое эхо
        const int delay = 16;
        for (int i = delay; i < samples; ++i) {
            int32_t s = audio[i] + audio[i - delay] / 5;
            audio[i] = Clamp16(s);
        }
    }

    void ApplyMegaphone(uint16_t* sampleBuffer, int samples) {
        int16_t* audio = reinterpret_cast<int16_t*>(sampleBuffer);
        const float sampleRate = 22050.0f;

        // High-pass
        const float hpAlpha = 1.0f - expf(-2.0f * M_PI * 650.0f / sampleRate);
        float hpPrevIn = 0.0f, hpOut = 0.0f;

        for (int i = 0; i < samples; ++i) {
            float x = (float)audio[i];
            hpOut = (1.0f - hpAlpha) * (hpOut + x - hpPrevIn);
            hpPrevIn = x;
            audio[i] = Clamp16((int32_t)hpOut);
        }

        // Low-pass
        const float lpAlpha = 1.0f - expf(-2.0f * M_PI * 2600.0f / sampleRate);
        float lpOut = 0.0f;

        for (int i = 0; i < samples; ++i) {
            lpOut += lpAlpha * ((float)audio[i] - lpOut);
            audio[i] = Clamp16((int32_t)lpOut);
        }

        // Короткий металлический отзвук
        const int delay = 22;
        for (int i = delay; i < samples; ++i) {
            int32_t s = audio[i] + audio[i - delay] / 3;
            audio[i] = Clamp16(s);
        }

        // Жёсткий ограничитель
        for (int i = 0; i < samples; ++i) {
            float x = audio[i] / 32768.0f;
            x *= 3.2f;

            if (x > 0.72f) x = 0.72f;
            else if (x < -0.72f) x = -0.72f;

            audio[i] = (int16_t)(x * 32767.0f);
        }
    }

    void ApplyChipmunk(uint16_t* sampleBuffer, int samples) {
        // повышение тона (ускорение) — просто сдвигаем сэмплы, но это грубо
        int16_t* audio = reinterpret_cast<int16_t*>(sampleBuffer);
        for (int i = 0; i < samples / 2; ++i) {
            audio[i] = audio[i * 2];
        }
        // остаток заполняем нулями
        for (int i = samples / 2; i < samples; ++i) {
            audio[i] = 0;
        }
        // здесь samples должен уменьшиться, но мы не меняем размер буфера; пусть будет как есть
    }

    void ApplySlowMotion(uint16_t* sampleBuffer, int samples) {
        // понижение тона (растяжение) — повторяем каждый сэмпл дважды
        int16_t* audio = reinterpret_cast<int16_t*>(sampleBuffer);
        for (int i = samples - 1; i >= 0; --i) {
            audio[i * 2] = audio[i];
            audio[i * 2 + 1] = audio[i];
        }
        // опять же размер не меняем
    }

    static float s_ringPhases = 0.0f;
    static float s_hpPrevs = 0.0f;
    static float s_lpPrevs = 0.0f;

    void ApplyCombo(uint16_t* sampleBuffer, int samples) {
        int16_t* audio = reinterpret_cast<int16_t*>(sampleBuffer);

        const float sampleRate = 22050.0f;

        // Wet/dry: 0.0 = чистый голос, 1.0 = полный эффект
        // 0.55 — слышна речь, но эффект чувствуется
        const float wetMix = 0.55f;
        const float dryMix = 1.0f - wetMix;

        // --- 1. Кольцевая модуляция ---
        // 300 Гц вместо 800 — воспринимается как понижение, не повышение
        // Смешиваем с оригиналом, не заменяем полностью
        const float ringFreqs = 300.0f;
        const float ringSteps = (2.0f * M_PI * ringFreqs) / sampleRate;
        const float ringDepths = 0.5f; // насколько глубока модуляция (0–1)

        for (int i = 0; i < samples; ++i) {
            float mod = 1.0f - ringDepths + ringDepths * sinf(s_ringPhases);
            s_ringPhases += ringSteps;
            if (s_ringPhases > 2.0f * M_PI) s_ringPhases -= 2.0f * M_PI;

            int32_t dry = audio[i];
            int32_t wet = (int32_t)(audio[i] * mod);
            audio[i] = Clamp16((int32_t)(dry * dryMix + wet * wetMix));
        }

        // --- 2. Клиппирование — мягче ---
        const int32_t clipLevel = 14000; // было 10000 — меньше давит речь
        const float   driveGain = 1.6f;  // было 2.5 — разборчивее
        for (int i = 0; i < samples; ++i) {
            int32_t s = (int32_t)(audio[i] * driveGain);
            s = s > clipLevel ? clipLevel : (s < -clipLevel ? -clipLevel : s);
            audio[i] = Clamp16(s);
        }

        // --- 3. Квантование — мягче ---
        const int quant = 180; // было 300 — меньше зернистости
        for (int i = 0; i < samples; ++i) {
            int32_t s = audio[i];
            s = (s / quant) * quant;
            audio[i] = Clamp16(s);
        }

        // --- 4. Гребенчатый фильтр ---
        const int   combDelay = 44;
        const float combMix = 0.3f; // было 0.45 — меньше "каши"
        for (int i = combDelay; i < samples; ++i) {
            int32_t s = (int32_t)(audio[i] + audio[i - combDelay] * combMix);
            audio[i] = Clamp16(s);
        }

        // --- 5. Полосовой фильтр ---
        const float hpAlphas = 1.0f - expf(-2.0f * M_PI * 300.0f / sampleRate);
        float hpPrevs = s_hpPrevs, hpOuts = 0.0f;
        for (int i = 0; i < samples; ++i) {
            hpOuts = (1.0f - hpAlphas) * (hpOuts + audio[i] - hpPrevs);
            hpPrevs = audio[i];
            audio[i] = Clamp16((int32_t)hpOuts);
        }
        s_hpPrevs = hpPrevs;

        const float lpAlpha = 1.0f - expf(-2.0f * M_PI * 3500.0f / sampleRate);
        float lpPrevs = s_lpPrevs;
        for (int i = 0; i < samples; ++i) {
            lpPrevs += lpAlpha * (audio[i] - lpPrevs);
            audio[i] = Clamp16((int32_t)lpPrevs);
        }
        s_lpPrevs = lpPrevs;
    }

} // namespace AudioEffects