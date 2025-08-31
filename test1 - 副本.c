#include <stdio.h>
#include <string.h>
#include <windows.h>
#include <ctype.h>

#define MAX_KEYS 64

void set_capslock(BOOL on) {
    BOOL state = (GetKeyState(VK_CAPITAL) & 1) != 0;
    if (state != on) {
        // 切换 CapsLock
        keybd_event(VK_CAPITAL, 0, 0, 0);
        keybd_event(VK_CAPITAL, 0, KEYEVENTF_KEYUP, 0);
        Sleep(10);
    }
}

void send_input(INPUT *inputs, int count) {
    if (count > 0) SendInput(count, inputs, sizeof(INPUT));
}

// 采集并发送普通键（小写、数字、符号）
void play_normal(const char *notes, int n, BOOL keyup) {
    INPUT ev[MAX_KEYS * 2];
    int cnt = 0;
    for (int i = 0; i < n; ++i) {
        char ch = notes[i];
        SHORT vkPair;
        BYTE vkCode;
        BOOL needShift = FALSE;
        // 符号映射
        switch (ch) {
            case '!': vkCode = '1'; needShift = TRUE; break;
            case '@': vkCode = '2'; needShift = TRUE; break;
            case '#': vkCode = '3'; needShift = TRUE; break;
            case '$': vkCode = '4'; needShift = TRUE; break;
            case '%': vkCode = '5'; needShift = TRUE; break;
            case '^': vkCode = '6'; needShift = TRUE; break;
            case '&': vkCode = '7'; needShift = TRUE; break;
            case '*': vkCode = '8'; needShift = TRUE; break;
            case '(' : vkCode = '9'; needShift = TRUE; break;
            case ')' : vkCode = '0'; needShift = TRUE; break;
            default:
                if (!isupper(ch)) {
                    vkPair = VkKeyScan(ch);
                    vkCode = LOBYTE(vkPair);
                    if (HIBYTE(vkPair) & 1) needShift = TRUE;
                } else continue;
        }
        // 按下 Shift
        if (needShift && !keyup) {
            ev[cnt].type = INPUT_KEYBOARD;
            ev[cnt].ki.wVk = VK_SHIFT;
            ev[cnt].ki.dwFlags = 0;
            cnt++;
        }
        // 按下/松开主键
        ev[cnt].type = INPUT_KEYBOARD;
        ev[cnt].ki.wVk = vkCode;
        ev[cnt].ki.dwFlags = keyup ? KEYEVENTF_KEYUP : 0;
        cnt++;
        // 松开 Shift
        if (needShift && keyup) {
            ev[cnt].type = INPUT_KEYBOARD;
            ev[cnt].ki.wVk = VK_SHIFT;
            ev[cnt].ki.dwFlags = KEYEVENTF_KEYUP;
            cnt++;
        }
    }
    send_input(ev, cnt);
}

// 采集并发送大写键（CapsLock + 小写键）
void play_upper(const char *notes, int n, BOOL keyup) {
    INPUT ev[MAX_KEYS];
    int cnt = 0;
    for (int i = 0; i < n; ++i) {
        if (isupper(notes[i])) {
            SHORT vkPair = VkKeyScan(tolower(notes[i]));
            if (vkPair != -1) {
                ev[cnt].type = INPUT_KEYBOARD;
                ev[cnt].ki.wVk = LOBYTE(vkPair);
                ev[cnt].ki.dwFlags = keyup ? KEYEVENTF_KEYUP : 0;
                cnt++;
            }
        }
    }
    send_input(ev, cnt);
}

int main() {
    char score[4096];
    printf("请输入谱子: ");
    if (!fgets(score, sizeof(score), stdin)) return 1;
    printf("按 = 键演奏下一个，长按持续，松开释放，=不输出。\n");

    int len = strlen(score), idx = 0;
    BOOL lastEq = FALSE;
    while (1) {
        // 跳过 = 空格 换行
        while (idx < len && (score[idx] == '=' || isspace(score[idx]))) idx++;
        if (idx >= len) break;
        // 解析一个音符或和弦
        char notes[MAX_KEYS]; int nc = 0;
        if (score[idx] == '[') {
            int j = idx + 1;
            while (j < len && score[j] != ']' && nc < MAX_KEYS) {
                if (isprint(score[j])) notes[nc++] = score[j];
                j++;
            }
            idx = (score[j] == ']') ? j + 1 : j;
        } else {
            if (isprint(score[idx])) notes[nc++] = score[idx];
            idx++;
        }
        if (nc == 0) continue;
        unsigned long long t0, t1;
        while (1) {
            BOOL eq = (GetAsyncKeyState(VK_OEM_PLUS) & 0x8000) != 0;
            if (eq && !lastEq) { t0 = GetTickCount64(); break; }
            lastEq = eq; Sleep(5);
        }
        BOOL hasUpper = FALSE;
        for (int i = 0; i < nc; i++) if (isupper(notes[i])) { hasUpper = TRUE; break; }
        if (hasUpper) set_capslock(TRUE);
        play_upper(notes, nc, FALSE);
        play_normal(notes, nc, FALSE);
        // 长按检测
        while (1) {
            BOOL eq = (GetAsyncKeyState(VK_OEM_PLUS) & 0x8000) != 0;
            if (!eq) break;
            t1 = GetTickCount64(); if (t1 - t0 > 700) while(GetAsyncKeyState(VK_OEM_PLUS) & 0x8000) Sleep(5);
            Sleep(5);
        }
        play_normal(notes, nc, TRUE);
        play_upper(notes, nc, TRUE);
        if (hasUpper) set_capslock(FALSE);
    }
    printf("演奏完成!\n");
    return 0;

}
