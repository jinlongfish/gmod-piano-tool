#include <stdio.h>
#include <string.h>
#include <windows.h>
#include <ctype.h>

#define MAX_KEYS 64

// �л� CapsLock
void set_capslock(BOOL on) {
    BOOL state = (GetKeyState(VK_CAPITAL) & 1) != 0;
    if (state != on) {
        // �л� CapsLock
        keybd_event(VK_CAPITAL, 0, 0, 0);
        keybd_event(VK_CAPITAL, 0, KEYEVENTF_KEYUP, 0);
        Sleep(10);
    }
}

// �������Ͱ����¼�
void send_input(INPUT *inputs, int count) {
    if (count > 0) SendInput(count, inputs, sizeof(INPUT));
}

// �ɼ���������ͨ����Сд�����֡����ţ�
void play_normal(const char *notes, int n, BOOL keyup) {
    INPUT ev[MAX_KEYS * 2];
    int cnt = 0;
    for (int i = 0; i < n; ++i) {
        char ch = notes[i];
        SHORT vkPair;
        BYTE vkCode;
        BOOL needShift = FALSE;
        // ����ӳ��
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
        // ���� Shift
        if (needShift && !keyup) {
            ev[cnt].type = INPUT_KEYBOARD;
            ev[cnt].ki.wVk = VK_SHIFT;
            ev[cnt].ki.dwFlags = 0;
            cnt++;
        }
        // ����/�ɿ�����
        ev[cnt].type = INPUT_KEYBOARD;
        ev[cnt].ki.wVk = vkCode;
        ev[cnt].ki.dwFlags = keyup ? KEYEVENTF_KEYUP : 0;
        cnt++;
        // �ɿ� Shift
        if (needShift && keyup) {
            ev[cnt].type = INPUT_KEYBOARD;
            ev[cnt].ki.wVk = VK_SHIFT;
            ev[cnt].ki.dwFlags = KEYEVENTF_KEYUP;
            cnt++;
        }
    }
    send_input(ev, cnt);
}

// �ɼ������ʹ�д����CapsLock + Сд����
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
    printf("����������: ");
    if (!fgets(score, sizeof(score), stdin)) return 1;
    printf("�� = ��������һ���������������ɿ��ͷţ�=�������\n");

    int len = strlen(score), idx = 0;
    BOOL lastEq = FALSE;
    while (1) {
        // ���� = �ո� ����
        while (idx < len && (score[idx] == '=' || isspace(score[idx]))) idx++;
        if (idx >= len) break;
        // ����һ�����������
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
        // ���ؼ�� =
        unsigned long long t0, t1;
        while (1) {
            BOOL eq = (GetAsyncKeyState(VK_OEM_PLUS) & 0x8000) != 0;
            if (eq && !lastEq) { t0 = GetTickCount64(); break; }
            lastEq = eq; Sleep(5);
        }
        // �ж��Ƿ񺬴�д
        BOOL hasUpper = FALSE;
        for (int i = 0; i < nc; i++) if (isupper(notes[i])) { hasUpper = TRUE; break; }
        if (hasUpper) set_capslock(TRUE);
        // ���ʹ�д
        play_upper(notes, nc, FALSE);
        // ������ͨ
        play_normal(notes, nc, FALSE);
        // �������
        while (1) {
            BOOL eq = (GetAsyncKeyState(VK_OEM_PLUS) & 0x8000) != 0;
            if (!eq) break;
            t1 = GetTickCount64(); if (t1 - t0 > 700) while(GetAsyncKeyState(VK_OEM_PLUS) & 0x8000) Sleep(5);
            Sleep(5);
        }
        // �ͷ���ͨ
        play_normal(notes, nc, TRUE);
        // �ͷŴ�д
        play_upper(notes, nc, TRUE);
        if (hasUpper) set_capslock(FALSE);
    }
    printf("�������!\n");
    return 0;
}