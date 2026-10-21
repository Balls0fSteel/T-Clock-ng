#ifndef AUTOVERSION_H
#define AUTOVERSION_H
/* Note: to use integer defines as strings, use STR(), eg. STR(VER_REVISION) */
/**** Version ****/
#	define VER_MAJOR 2
#	define VER_MINOR 5
#	define VER_BUILD 0
	/** status values: 0=Alpha(α), 1=Beta(β), 2=RC(гc), 3=Release(г), 4=Maintenance(гm) */
#	define VER_STATUS 3
#	define VER_STATUS_FULL "Release"
#	define VER_STATUS_SHORT "r"
#	define VER_STATUS_GREEK "\u0433"
#	define VER_REVISION 537
#	define VER_FULL "2.5.0 Release"
#	define VER_SHORT "2.5r0"
#	define VER_SHORT_DOTS "2.5.0"
#	define VER_SHORT_GREEK "2.5\u04330"
#	define VER_RC_REVISION 2, 5, 0, 537
#	define VER_RC_STATUS 2, 5, 0, 3
/**** Subversion Information ****/
#	define VER_REVISION_URL "https://github.com/Balls0fSteel/T-Clock-ng"
#	define VER_REVISION_DATE "2026-10-12 15:40:30 +0000 (Mon, Oct 12 2026)"
#	define VER_REVISION_HASH "3878ca3"
#	define VER_REVISION_TAG "v2.5.0#537-release"
/**** Date/Time ****/
#	define VER_TIMESTAMP 1792614775
#	define VER_TIME_SEC 55
#	define VER_TIME_MIN 32
#	define VER_TIME_HOUR 20
#	define VER_TIME_DAY 21
#	define VER_TIME_MONTH 10
#	define VER_TIME_YEAR 2026
#	define VER_TIME_WDAY 3
#	define VER_TIME_YDAY 293
#	define VER_TIME_WDAY_SHORT "Wed"
#	define VER_TIME_WDAY_FULL "Wednesday"
#	define VER_TIME_MONTH_SHORT "Oct"
#	define VER_TIME_MONTH_FULL "October"
#	define VER_TIME "20:32:55"
#	define VER_DATE "2026-10-21"
#	define VER_DATE_LONG "Wed, Oct 21, 2026 20:32:55 UTC"
#	define VER_DATE_SHORT "2026-10-21 20:32:55 UTC"
#	define VER_DATE_ISO "2026-10-21T20:32:55Z"
/**** Helper 'functions' ****/
#	define VER_IsReleaseOrHigher() ( VER_STATUS >= 3 )
#	define VER_IsAlpha() ( VER_STATUS == 0 )
#	define VER_IsBeta() ( VER_STATUS == 1 )
#	define VER_IsRC() ( VER_STATUS == 2 )
#	define VER_IsRelease() ( VER_STATUS == 3 )
#	define VER_IsMaintenance() ( VER_STATUS == 4 )
#ifndef STR
#	define STR_(x) #x
#	define STR(x) STR_(x)
#endif
#ifndef L
#	define L_(x) L##x
#	define L(x) L_(x)
#endif
#endif
