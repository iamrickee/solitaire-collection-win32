#pragma once

// Bitmaps / icons / menus / accelerators
#define IDI_APP                 100     // lowest icon ID = the exe's icon in Explorer
#define IDB_CARDS               101
#define IDR_MAINMENU            102
#define IDR_ACCEL               103

// Dialogs
#define IDD_ABOUT               200
#define IDD_SETTINGS            201
#define IDD_HIGHSCORES          202
#define IDD_DEAL                203
#define IDD_RULES               204
#define IDD_KLONDIKE_OPTIONS    210
#define IDD_SPIDER_OPTIONS      211

// Settings dialog controls
#define IDC_BG_BUTTON           1001
#define IDC_BG_SWATCH           1002
#define IDC_BACK_BUTTON         1003
#define IDC_BACK_SWATCH         1004
#define IDC_FOURCOLOR           1005
#define IDC_DEFAULTS            1006

// High scores dialog controls
#define IDC_SCORELIST           1101
#define IDC_SCORERESET          1102

// Deal number dialog controls
#define IDC_DEAL_NUMBER         1401

// How to Play dialog controls
#define IDC_RULES_GAMES         1501
#define IDC_RULES_TEXT          1502

// Klondike options controls
#define IDC_KL_DRAW1            1201
#define IDC_KL_DRAW3            1202
#define IDC_KL_STANDARD         1203
#define IDC_KL_VEGAS            1204
#define IDC_KL_CUMULATIVE       1205
#define IDC_KL_RESETBANK        1206

// Spider options controls
#define IDC_SP_SUIT1            1301
#define IDC_SP_SUIT2            1302
#define IDC_SP_SUIT4            1303
// Menu commands
#define IDM_NEW                 40001
#define IDM_RESTART             40002
#define IDM_UNDO                40003
#define IDM_AUTOPLAY            40004
#define IDM_OPTIONS             40005
#define IDM_SETTINGS            40006
#define IDM_HIGHSCORES          40007
#define IDM_EXIT                40008
#define IDM_ABOUT               40009
#define IDM_GAMES_PLACEHOLDER   40010
#define IDM_NEWDEAL             40011
#define IDM_RULES               40012
#define IDM_GAME_FIRST          41000   // 41000 + index into AllGames()
