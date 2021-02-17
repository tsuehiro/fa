// 2011 01.28 少し動くようにしてみようかとおもったけど、めんどい。。。
// 2016 07.05 した

#include	<stdio.h>
#include	<stdlib.h>
#include	<sys/types.h>
#include	<sys/stat.h>
#include	<sys/param.h>
#include	<sys/mount.h>
#include	<dirent.h>
#include	<curses.h>
#include	<unistd.h>
#include	<string.h>
#include	<time.h>
#include	<pwd.h>

//	linux
#include <sys/vfs.h>

//#define	OK	0
//#define	ERR	-1
#define	ON	0
#define	OFF	-1

#define	DEF_MAXFILES	10000
#define	FA_VERSION	"0.02"

#define	DEBUG	OFF

void	Debug( char* as_Str )
{
	FILE	*fp;

	fp = fopen( "/tmp/fa.log", "at" );
	fprintf( fp, "%s\n", as_Str );
	fclose( fp );
}
//linux
char*	user_from_uid( uid_t at_uid, int ai_dummy )
{
	struct passwd	*info;

	info = getpwuid( at_uid );

	return( info->pw_name );
}

struct FileInfo	{
public:
	char		*s_FileName;
	struct stat	*Stat;
	int		i_Flag;

	FileInfo(void);
	FileInfo( char* );
	~FileInfo( void );
int	ReadInfo( char* );
int	SetFlag( int );
int	GetFlag( void );
};
	FileInfo::FileInfo( void )
{
	i_Flag = 0;
}
	FileInfo::FileInfo( char* FileName )
{
	ReadInfo( FileName );
}
int	FileInfo::ReadInfo( char* FileName )
{
	int		i_Len;

	// ファイル名保存
	i_Len = strlen(FileName)+1;
	s_FileName = new char[ i_Len ];
	strcpy( s_FileName, FileName );

	// 属性
	Stat = new struct stat;
	stat( FileName, Stat );

	// フラグ
	SetFlag( 0 );

	return( OK );
}
	FileInfo::~FileInfo( void )
{
	if ( Stat != NULL )	{
		delete( Stat );
	}
	if ( s_FileName != NULL )	{
		delete( s_FileName );
	}
}
int	FileInfo::SetFlag( int ai_Flag )
{
	i_Flag = ai_Flag;

	return( OK );
}
int	FileInfo::GetFlag( void )
{
	return( i_Flag );
}

struct Dir_Entry	{
public:
	int		i_FileNo;
	char		*s_DirPath;
	struct FileInfo	*f_File;
	struct statfs	f_StatFs;

	Dir_Entry( void );
	Dir_Entry( char* );
	~Dir_Entry( void );
int	Init( void );
int	Load_Entry( char* );
int	Reload_Entry( void );
int	SetFlag( int, int );
int	GetFlag( int );
char*	GetFileName( int );
int	GetFileSize( int );
int	GetFileStat( int );
const	char*	GetUserName( int );
int	GetLastUpdate_ttime( int );
int	GetLastUpdate_Ascii( int, char* );
int	GetDiskFree( char* );	// 引数エリアに書き込む
int	Debug_Info( void );
};
	Dir_Entry::Dir_Entry( void )
{
	Init();
}
	Dir_Entry::Dir_Entry( char* as_DirPath )
{
	Init();
	Load_Entry( as_DirPath );
}
	Dir_Entry::~Dir_Entry( void )
{
	delete( f_File );
	delete( s_DirPath );
}
int	Dir_Entry::Init( void )
{
	//	初期化
	i_FileNo	= 0;
	f_File		= new struct FileInfo[ DEF_MAXFILES ];
//	f_StatFs	= new struct statfs;

	s_DirPath	= new char[ 4096 ];

	return( OK );
}
int	Dir_Entry::Reload_Entry( void )
{
	Load_Entry( s_DirPath );

	return( OK );
}
int	Dir_Entry::Load_Entry( char* DirPath )
{
	struct	dirent	*de;
	DIR		*dp;

	statfs( DirPath, &f_StatFs );
	chdir( DirPath );
	getcwd( s_DirPath, 4096 );

	i_FileNo = 0;
	dp = opendir(".");
	if ( !dp )	{
		i_FileNo = -1;
		return( ERR );
	}
	while ( 1 )	{
		de = readdir(dp);
		if ( de == NULL ) {
			break;
		}
		if ( strcmp( de->d_name, "." ) == 0 )	{
			continue;
		}
//		if ( strcmp( de->d_name, ".." ) == 0 )	{
//			continue;
//		}
		f_File[ i_FileNo ].ReadInfo( de->d_name );
		i_FileNo++;
	}
	closedir(dp);
	return( OK );
}
int	Dir_Entry::SetFlag( int ai_Target, int ai_Value )
{
	// ..は選択できないようにする除く
	if ( strcmp( GetFileName( ai_Target ), ".." ) ) {
		f_File[ ai_Target ].SetFlag( ai_Value );
	}

	return( OK );
}
int	Dir_Entry::GetFlag( int ai_Target )
{
	return( f_File[ ai_Target ].GetFlag() );
}
char*	Dir_Entry::GetFileName( int ai_Target )
{
	return( f_File[ ai_Target ].s_FileName );
}
int	Dir_Entry::GetFileSize( int ai_Target )
{
	return( f_File[ ai_Target ].Stat->st_size );
}
int	Dir_Entry::GetFileStat( int ai_Target )
{
	return( f_File[ ai_Target ].Stat->st_mode );
}
const	char*	Dir_Entry::GetUserName( int ai_Target )
{
	const char *username;

	username = user_from_uid( f_File[ ai_Target ].Stat->st_uid, 0 );

	return( username );
}
int	Dir_Entry::GetLastUpdate_ttime( int ai_Target )
{
	return( f_File[ ai_Target ].Stat->st_mtime );
}
int	Dir_Entry::GetLastUpdate_Ascii( int ai_Target, char* as_Target )
{
	char	s_WorkBuf[ 32 ];
	struct	tm*	tm;

	tm = localtime( &f_File[ ai_Target ].Stat->st_mtime );
	sprintf( as_Target, " %02d/%02d/%02d %02d:%02d:%02d",
				tm->tm_year-100,	tm->tm_mon+1,	tm->tm_mday,
				tm->tm_hour,		tm->tm_min,	tm->tm_sec );

	return( OK );
}
int	Dir_Entry::GetDiskFree( char* ap_StrBuf )
{
	double	d_Free;
	int	i_Col;

	d_Free = (double)(f_StatFs.f_bfree) * ((double)f_StatFs.f_bsize);
	sprintf( ap_StrBuf, "%3.2f Bytes", d_Free );
	if ( d_Free > 1000.0 )	{
		sprintf( ap_StrBuf, "%3.2f KBytes", d_Free/1000.0 );
	}
	if ( d_Free > 1000000.0 )	{
		sprintf( ap_StrBuf, "%3.2f MBytes", d_Free/1000000.0 );
	}
	if ( d_Free > 1000000000.0 )	{
		sprintf( ap_StrBuf, "%3.2f GBytes", d_Free/1000000000.0 );
	}

	return( OK );
}
int	Dir_Entry::Debug_Info( void )
{
	int	i_Cnt;

	printf( "登録ディレクトリ:%s(%d個)\n", s_DirPath, i_FileNo );
	for ( i_Cnt = 0 ; i_Cnt <= (i_FileNo-1) ; i_Cnt++ )	{
		printf( "%s\t%d\t%d\n",
				f_File[i_Cnt].s_FileName,
				(int)(f_File[i_Cnt].Stat->st_size),
				(int)(f_File[i_Cnt].Stat->st_mode) );
	}

	return( OK );
}

struct	Dir_Window	{
public:
	int			i_PosX, i_PosY;		//	ウィンドウの左上位置
	int			i_MaxX, i_MaxY;		//	ウィンドウのサイズ
	int			i_Offset;		//	表示オフセット位置
	int			i_CsrPtr;		//	カーソル位置
	int			i_ActiveFlag;		//	フォーカスがあれば0以外
	WINDOW*			h_WIN;
	struct	Dir_Entry	*Entry;

	Dir_Window( void );
	Dir_Window( char* );
	Dir_Window( int, int, int, int );
	Dir_Window( int, int, int, int, char* );

int	Init( void );
int	SetWindow( int, int, int, int );
int	ResizeWindow( int, int, int, int );
//int	SetWindowPosition( int, int );
//int	SetWindowSize( int, int );
int	Load_Entry( char* );
int	Reload_Entry( void );
int	Reload_Entry( char* );
int	ChDir( char* );
int	Refresh( void );
int	CsrUp( void );
int	CsrDown( void );
int	SetFlag( int );
int	SetFlag( int, int );
int	GetFlag( void );
int	GetFlag( int );
int	GetItemNum( void );
char*	GetFileName( void );
char*	GetFileName( int );
int	GetFileStat( void );
int	GetFileStat( int );
//char*	GetUserName( void );
//char*	GetUserName( int );
int	SetActiveFlag( int );
int	SetActive( void );
int	LostActive( void );
};
	Dir_Window::Dir_Window( void )
{
	Init();
	Entry = new struct Dir_Entry;
}
	Dir_Window::Dir_Window( char* as_DirPath )
{
	Init();
	Entry = new struct Dir_Entry;
	Reload_Entry( as_DirPath );
}
	Dir_Window::Dir_Window( int ai_PosX, int ai_PosY, int ai_MaxX, int ai_MaxY )
{
	Init();
	SetWindow( ai_PosX, ai_PosY, ai_MaxX, ai_MaxY );
	Entry = new struct Dir_Entry;
}
	Dir_Window::Dir_Window( int ai_PosX, int ai_PosY, int ai_MaxX, int ai_MaxY, char* as_DirPath )
{
	Init();
	SetWindow( ai_PosX, ai_PosY, ai_MaxX, ai_MaxY );
	Entry = new struct Dir_Entry;
	Reload_Entry( as_DirPath );
}
int	Dir_Window::Init( void )
{
	i_PosX		= 0;	i_PosY =	0;
	i_MaxX		= 0;	i_MaxY =	0;
	i_Offset	= 0;
	i_CsrPtr	= 0;
	i_ActiveFlag	= 0;

	return( OK );
}
//int	Dir_Window::SetWindowPosition( int ai_PosX, int ai_PosY )
//{
//	i_PosX = ai_PosX;	i_PosY = ai_PosY;
//
//	return( OK );
//}
//int	Dir_Window::SetWindowSize( int ai_MaxX, int ai_MaxY )
//{
//	i_MaxX = ai_MaxX;	i_MaxY = ai_MaxY;
//
//	return( OK );
//}
int	Dir_Window::ResizeWindow( int ai_PosX, int ai_PosY, int ai_MaxX, int ai_MaxY )
{
	i_PosX = ai_PosX;	i_PosY = ai_PosY;
	i_MaxX = ai_MaxX;	i_MaxY = ai_MaxY;
	delwin( h_WIN );
	h_WIN = newwin( i_MaxY, i_MaxX, i_PosY, i_PosX );

	return( OK );
}
int	Dir_Window::SetWindow( int ai_PosX, int ai_PosY, int ai_MaxX, int ai_MaxY )
{
	i_PosX = ai_PosX;	i_PosY = ai_PosY;
	i_MaxX = ai_MaxX;	i_MaxY = ai_MaxY;
	h_WIN = newwin( i_MaxY, i_MaxX, i_PosY, i_PosX );

	return( OK );
}
int	Dir_Window::Reload_Entry( void )
{
	Entry->Reload_Entry();
	if ( i_CsrPtr > (GetItemNum()-1) )	{
		i_CsrPtr = GetItemNum()-1;
	}

	return( OK );
}
int	Dir_Window::Reload_Entry( char* as_DirPath )
{
	Entry->Load_Entry( as_DirPath );
	if ( i_CsrPtr > (GetItemNum()-1) )	{
		i_CsrPtr = GetItemNum()-1;
	}

	return( OK );
}
int	Dir_Window::ChDir( char* as_DirPath )
{
	Entry->Load_Entry( as_DirPath );
	if ( i_CsrPtr > (GetItemNum()-1) )	{
		i_CsrPtr = GetItemNum()-1;
	}

	i_CsrPtr = 0;
	i_Offset = 0;

	return( OK );
}
int	Dir_Window::CsrUp( void )
{
	i_CsrPtr--;
	if ( i_CsrPtr < 0 )	{
		i_CsrPtr = 0;
	}

	if ( (i_CsrPtr-i_Offset) < 5 )	{
		i_Offset = i_CsrPtr - 5;
	}
	if ( i_Offset < 0 )	{
		i_Offset = 0;
	}

	return( OK );
}
int	Dir_Window::CsrDown( void )
{
	i_CsrPtr++;
	if ( i_CsrPtr > (Entry->i_FileNo-1) )	{
		i_CsrPtr = Entry->i_FileNo-1;
	}

	if ( (i_CsrPtr-i_Offset) > (i_MaxY-7 ) )	{
		i_Offset = i_CsrPtr - (i_MaxY-7);
	}
	//	これうまく式が作れない...
//	if ( i_Offset > (GetItemNum()-(i_MaxY-7)) )	{
//		i_Offset = GetItemNum()-(i_MaxY-7);
//	}
	if ( i_Offset < 0 )	{
		i_Offset = 0;
	}

	return( OK );
}
int	Dir_Window::SetFlag( int ax_Value )
{
	Entry->SetFlag( i_CsrPtr, ax_Value );
	return( OK );
}
int	Dir_Window::SetFlag( int ax_Target, int ax_Value )
{
	Entry->SetFlag( ax_Target, ax_Value );
	return( OK );
}
int	Dir_Window::GetFlag( void )
{
	return( Entry->GetFlag( i_CsrPtr ) );
}
int	Dir_Window::GetFlag( int ax_Target )
{
	return( Entry->GetFlag( ax_Target ) );
}
int	Dir_Window::GetItemNum( void )
{
	return( Entry->i_FileNo );
}
char*	Dir_Window::GetFileName( void )
{
	return( Entry->GetFileName(i_CsrPtr) );
}
char*	Dir_Window::GetFileName( int ai_Target )
{
	return( Entry->GetFileName(ai_Target) );
}
int	Dir_Window::GetFileStat( void )
{
	return( Entry->GetFileStat(i_CsrPtr) );
}
int	Dir_Window::GetFileStat( int ai_Target )
{
	return( Entry->GetFileStat(ai_Target) );
}
//char*	Dir_Window::GetUserName( void )
//{
//	return( Entry->GetUserName(i_CsrPtr) );
//}
//char*	Dir_Window::GetUserName( int ai_Target )
//{
//	return( Entry->GetUserName(ai_Target) );
//}
int	Dir_Window::SetActiveFlag( int ax_Value )
{
	i_ActiveFlag = ax_Value;
	return( OK );
}
int	Dir_Window::SetActive( void )
{
	i_ActiveFlag = 1;
	return( OK );
}
int	Dir_Window::LostActive( void )
{
	i_ActiveFlag = 0;
	return( OK );
}
int	Dir_Window::Load_Entry( char* as_DirPath )
{
	Entry->Load_Entry( as_DirPath );
	return( OK );
}
int	Dir_Window::Refresh( void )
{
	int	i_Cnt, i_Cnt2;
	char	s_WorkBuf[ 4096 ];
	char	s_WorkBuf2[ 4096 ];
	char	s_FileName[ 4096 ];
	char	s_Info1[ 4096 ];
	char	s_Info2[ 4096 ];
	char	s_Info3[ 4096 ];

	sprintf( s_WorkBuf, "- %s -", Entry->s_DirPath );
	mvwaddstr( h_WIN, 0,  1, s_WorkBuf );
	wclrtoeol( h_WIN );
	for ( i_Cnt = 0 ; i_Cnt < (i_MaxY-3) ; i_Cnt++ )	{
		// 表示の必要がない行を消す(※表示位置が変わった場合要注意)
		if ( (i_Cnt+i_Offset) >= GetItemNum() )	{
			wmove( h_WIN, i_Cnt+1, 0 );
			wclrtoeol( h_WIN );
			continue;
		}
		if ( ( (i_Cnt+i_Offset) == i_CsrPtr) && ( i_ActiveFlag != 0 ) )	{
			wattron(h_WIN, A_UNDERLINE);
		}
		if ( Entry->GetFlag(i_Cnt+i_Offset) == 1 )	{
			wattron(h_WIN, A_REVERSE);
		}

		sprintf( s_WorkBuf2, "\%%-%ds", i_MaxX-1 );
		sprintf( s_FileName, s_WorkBuf2, Entry->GetFileName( i_Cnt+i_Offset ) );
		s_FileName[i_MaxX-10 ] = 0;	// 切り取り
//		if ( strcmp( Entry->GetFileName( i_Cnt+i_Offset ), ".." ) == 0 )	{
//			sprintf( s_FileName, s_WorkBuf2, " ===== < Parent Dir > ===== " );
//		}	else	{
//			sprintf( s_FileName, s_WorkBuf2, Entry->GetFileName( i_Cnt+i_Offset ) );
//		}

		if ( S_ISDIR( Entry->GetFileStat(i_Cnt+i_Offset) ) )	{
			sprintf( s_Info1, "%10s", "<DIR>" );
		}	else	{
			sprintf( s_Info1, "%10d", Entry->GetFileSize( i_Cnt+i_Offset ) );
		}

		sprintf( s_Info2, "" );
		sprintf( s_Info3, "" );
		if ( i_MaxX > 24 )	{
			Entry->GetLastUpdate_Ascii( i_Cnt+i_Offset, (char*)&s_Info2 );
			s_FileName[i_MaxX-(10+16) ] = 0;	// 切り取り
		}	else	{
			s_FileName[i_MaxX-(10) ] = 0;	// 切り取り
		}


		if ( i_MaxX > 40 )	{
			sprintf( s_Info3, " %-10s", Entry->GetUserName(i_Cnt+i_Offset) );
			s_FileName[i_MaxX-(10+16+10) ] = 0;	// 切り取り
		}	else	{
			s_FileName[i_MaxX-(10+16) ] = 0;	// 切り取り
		}

		sprintf( s_WorkBuf, "%s%s%s%s", s_FileName, s_Info1, s_Info2, s_Info3 );
		for ( i_Cnt2 = strlen(s_WorkBuf) ; i_Cnt2 < (i_MaxX-1) ; i_Cnt2++ )	{
			strcat( s_WorkBuf, " " );
		}
		s_WorkBuf[i_MaxX-1 ] = 0;	// 切り取り

		mvwaddstr( h_WIN, i_Cnt+1, 1, s_WorkBuf );
		wattroff( h_WIN, A_UNDERLINE );
		wattroff( h_WIN, A_REVERSE );
		wclrtoeol( h_WIN );
	}

	Entry->GetDiskFree( s_WorkBuf2 );
	sprintf( s_WorkBuf, "%6d/%6d files | Free %s", i_CsrPtr, Entry->i_FileNo-1, s_WorkBuf2 );
	mvwaddstr( h_WIN, i_MaxY-1,  1, s_WorkBuf );
	wclrtoeol( h_WIN );

	if ( DEBUG == ON )	{
		mvwprintw( h_WIN, i_MaxY-1,  0,  "%-4d", i_CsrPtr );
		mvwprintw( h_WIN, i_MaxY-1, 10,  "%-4d", i_Offset );
		mvwprintw( h_WIN, i_MaxY-1, 20, "%-20s", Entry->GetFileName(i_CsrPtr) );
		mvwprintw( h_WIN, i_MaxY-1, 40,  "%-5d", Entry->GetFileStat(i_CsrPtr) );
//		mvwprintw( h_WIN, 0, 20, "%-4d", i_Accel_Time );
//		mvwprintw( h_WIN, 0, 30, "%-4d", i_CsrPtr[0] );
//		mvwprintw( h_WIN, 0, 35, "%-4d", i_WinOffset[0] );
//		mvwprintw( h_WIN, 0, 40, "%-4d", i_CsrPtr[1] );
//		mvwprintw( h_WIN, 0, 45, "%-4d", i_WinOffset[1] );
//		mvwprintw( h_WIN, 1+i_Cnt, 40, "%d", dir1->Stat[i+i_offset]->st_size );
	}
	wrefresh( h_WIN );

	return( OK );
}

int	main( void )
{
	int			i_Ch;
	int			i_Cnt, i_Cnt2;
	int			i_CsrPtr[2], i_WinOffset[2];
	int			i_CsrWin, i_Win;
	int			i_MaxX, i_MaxY;
	int			i_Repeat_Cnt, i_Repeat_Cnt2, i_Repeat_Char, i_Accel_Time;
	int			i_Debug_Flg = 0;
	int			i_Flag;
	int			i_Res;
	char			*s_CmdPath;
	char			*s_WorkBuf, *s_WorkBuf2;
	char			*s_StartTime_CurrentDir;
	struct	Dir_Window	*Win[10];

	// 作業用バッファ
	s_WorkBuf  = new char[ 4096 ];
	s_WorkBuf2 = new char[ 4096 ];

	// 初期化
	initscr();
	start_color();
	init_pair(1, COLOR_WHITE, COLOR_BLACK);
//	init_pair(2, COLOR_WHITE, COLOR_BLUE);
	init_pair(2, COLOR_CYAN,  COLOR_BLUE );

	// コントロールキー、バックスペース等の制御をしない
	cbreak();

	// エコーオフ
	noecho();

	// 入力のタイムアウト設定(ms)
	timeout(100);

	// 矢印等の入力許可
	keypad(stdscr,TRUE);

	// 画面サイズ取得
	i_MaxX = getmaxx(stdscr);	i_MaxY = getmaxy(stdscr);

	wrefresh(stdscr);

	s_StartTime_CurrentDir  = new char[ 4096 ];
	getcwd( s_StartTime_CurrentDir, 4096 );

	strcpy( s_WorkBuf, "." );
	if ( getenv( "FADIR1" ) != NULL )	{
		strcpy( s_WorkBuf, getenv( "FADIR1" ) );
	}
	Win[0] = new Dir_Window(          1, 1, (i_MaxX/2)-2, i_MaxY-2, s_WorkBuf );

	chdir( s_StartTime_CurrentDir );
	strcpy( s_WorkBuf, "." );
	if ( getenv( "FADIR2" ) != NULL )	{
		strcpy( s_WorkBuf, getenv( "FADIR2" ) );
	}
	Win[1] = new Dir_Window( (i_MaxX/2), 1, (i_MaxX/2)-2, i_MaxY-2, s_WorkBuf );
	Win[0]->SetActive();


	// カーソル関連初期化
	i_CsrWin = 0;	// 左
	i_Repeat_Cnt = 0; i_Repeat_Cnt2 = 0; i_Repeat_Char = 0; i_Accel_Time = 1;

	for ( ; ; )	{

		Win[0]->Refresh();
		Win[1]->Refresh();

		// 外枠
		sprintf( s_WorkBuf, "   FA %s", FA_VERSION );
//		attron(A_REVERSE);
		for ( i_Cnt2 = strlen(s_WorkBuf) ; i_Cnt2 < (i_MaxX) ; i_Cnt2++ )	{
			strcat( s_WorkBuf, " " );
		}
		attrset(COLOR_PAIR(2) | A_REVERSE );	
		mvwaddstr(stdscr, 0, 0, s_WorkBuf );
		attrset(COLOR_PAIR(1));	

		sprintf( s_WorkBuf, "" );
		for ( i_Cnt2 = strlen(s_WorkBuf) ; i_Cnt2 < (i_MaxX) ; i_Cnt2++ )	{
			strcat( s_WorkBuf, " " );
		}
		mvwaddstr(stdscr, i_MaxY-1, 0, s_WorkBuf );
		attroff(A_REVERSE);
		refresh();

		i_Ch = wgetch(stdscr);
		if ( DEBUG == ON )	{
			sprintf( s_WorkBuf, "%-10d", i_Ch );
			mvwaddstr(stdscr, i_MaxY-1, 0, s_WorkBuf );
			refresh();
			usleep( 1000 );
		}
		if ( i_Ch >= 0 )	{
			if ( i_Repeat_Char == i_Ch )	{
			// キー入力があったとき
				i_Repeat_Cnt++;
			}	else	{
				i_Repeat_Cnt = 0;
			}
			i_Repeat_Char = i_Ch;
			i_Repeat_Cnt2 = 0;
		}	else	{
			// キー入力が無いとき
			usleep( 1000 );
			i_Repeat_Cnt2++;
			if ( i_Repeat_Cnt2 > 5 )	{
				i_Repeat_Cnt = 0; i_Repeat_Cnt2 = 0;
				// sleep状態?
				int	tmpi_MaxX, tmpi_MaxY;

				tmpi_MaxX = getmaxx(stdscr);
				tmpi_MaxY = getmaxy(stdscr);
				if ( (i_MaxX!=tmpi_MaxX) || (i_MaxY!=tmpi_MaxY) )	{
					i_MaxX = tmpi_MaxX;	i_MaxY = tmpi_MaxY;
					Win[0]->ResizeWindow(          1, 1, (i_MaxX/2)-2, i_MaxY-2 );
					Win[1]->ResizeWindow( (i_MaxX/2), 1, (i_MaxX/2)-2, i_MaxY-2 );
					clear();
					Win[0]->Refresh();
					Win[1]->Refresh();
					refresh();
				}
			}
		}
		if ( i_Repeat_Cnt < 20 )	{
			i_Accel_Time = 1;
		}
		if ( i_Repeat_Cnt >= 20 )	{
			i_Accel_Time = 2;
		}
		if ( i_Repeat_Cnt >= 50 )	{
			i_Accel_Time = 4;
		}

		if ( i_Ch == 'q' )	{
			break;
		}

		switch ( i_Ch )	{
			case 'J':
				for ( i_Cnt = 1 ; i_Cnt <= (i_Accel_Time*10) ; i_Cnt++ )	{
					Win[i_CsrWin]->CsrDown();
				}
				break;
			case 'K':
				for ( i_Cnt = 1 ; i_Cnt <= (i_Accel_Time*10) ; i_Cnt++ )	{
					Win[i_CsrWin]->CsrUp();
				}
				break;
//			case 'U':
//				i_CsrWin = 1;
//				Win[1]->CsrDown();
//				Win[1]->SetActive();
//				Win[0]->LostActive();
//				break;
//			case 'I':
//				i_CsrWin = 1;
//				Win[1]->CsrUp();
//				Win[1]->SetActive();
//				Win[0]->LostActive();
//				break;
			case 258:	//	↓
			case 14:	//	CTRL+N
			case 'j':
				for ( i_Cnt = 1 ; i_Cnt <= i_Accel_Time ; i_Cnt++ )	{
					Win[i_CsrWin]->CsrDown();
				}
				break;
			case 259:	//	↑
			case 16:	//	CTRL+P
			case 'k':
				for ( i_Cnt = 1 ; i_Cnt <= i_Accel_Time ; i_Cnt++ )	{
					Win[i_CsrWin]->CsrUp();
				}
				break;
			case 2:		//	←
			case 'h':
				i_CsrWin = 0;
				Win[0]->SetActive();
				Win[1]->LostActive();
				break;
			case 6:		//	→
			case 'l':
				i_CsrWin = 1;
				Win[1]->SetActive();
				Win[0]->LostActive();
				break;
			case ' ':
				Win[i_CsrWin]->SetFlag( Win[i_CsrWin]->GetFlag() ^ 1 );
				Win[i_CsrWin]->CsrDown();
				break;
			case '?':
				Win[i_CsrWin]->Entry->Debug_Info();
				refresh();
				break;
			case 1:		// CTRL+A
				i_Flag = 1;
				for ( i_Cnt = 0 ; i_Cnt < Win[i_CsrWin]->GetItemNum() ; i_Cnt++ )	{
					if ( ( Win[i_CsrWin]->GetFlag(i_Cnt) & 1 ) == 0 )	{
						i_Flag = 0;
					}
				}
				if ( i_Flag == 0 )	{
					for ( i_Cnt = 0 ; i_Cnt < Win[i_CsrWin]->GetItemNum() ; i_Cnt++ )	{
						Win[i_CsrWin]->SetFlag( i_Cnt, 1 );
					}
				}	else	{
					for ( i_Cnt = 0 ; i_Cnt < Win[i_CsrWin]->GetItemNum() ; i_Cnt++ )	{
						Win[i_CsrWin]->SetFlag( i_Cnt, 0 );
					}
				}
				break;
			case 'a':
				for ( i_Cnt = 0 ; i_Cnt < Win[i_CsrWin]->GetItemNum() ; i_Cnt++ )	{
					Win[i_CsrWin]->SetFlag( i_Cnt, Win[i_CsrWin]->GetFlag(i_Cnt) ^ 1 );
				}
				break;
			case 'd':
				i_Flag = 0;
				for ( i_Cnt = 0 ; i_Cnt < Win[i_CsrWin]->GetItemNum() ; i_Cnt++ )	{
					if ( ( Win[i_CsrWin]->GetFlag(i_Cnt) & 1 ) == 1 )	{
						i_Flag = 1;
					}
				}
				s_CmdPath = new char[ 4096 ];
				if ( i_Flag == 0 )	{
					sprintf( s_CmdPath, "mv '%s' '/tmp/Trash'",
							Win[i_CsrWin]->GetFileName() );
					Debug( s_CmdPath );
					system( s_CmdPath );
				}	else	{
					for ( i_Cnt = 0 ; i_Cnt < Win[i_CsrWin]->GetItemNum() ; i_Cnt++ )	{
						if ( ( Win[i_CsrWin]->GetFlag(i_Cnt) & 1 ) == 1 )	{
							sprintf( s_CmdPath, "mv '%s' '/tmp/Trash'",
									Win[i_CsrWin]->GetFileName(i_Cnt) );
							Debug( s_CmdPath );
							system( s_CmdPath );
						}
					}
				}
				Win[0]->Reload_Entry();
				Win[1]->Reload_Entry();
				delete( s_CmdPath );
				break;
			case 'm':
				s_CmdPath = new char[ 4096 ];
				i_Flag = 0;
				for ( i_Cnt = 0 ; i_Cnt < Win[i_CsrWin]->GetItemNum() ; i_Cnt++ )	{
					if ( ( Win[i_CsrWin]->GetFlag(i_Cnt) & 1 ) == 1 )	{
						i_Flag = 1;
					}
				}
				if ( i_Flag == 0 )	{
					sprintf( s_CmdPath, "mv '%s' '%s'",
							Win[i_CsrWin]->GetFileName(),
							Win[i_CsrWin^1]->Entry->s_DirPath );
					Debug( s_CmdPath );
					system( s_CmdPath );
				}	else	{
					for ( i_Cnt = 0 ; i_Cnt < Win[i_CsrWin]->GetItemNum() ; i_Cnt++ )	{
						if ( ( Win[i_CsrWin]->GetFlag(i_Cnt) & 1 ) == 1 )	{
							sprintf( s_CmdPath, "mv '%s' '%s'",
									Win[i_CsrWin]->GetFileName(i_Cnt),
									Win[i_CsrWin^1]->Entry->s_DirPath );
							Debug( s_CmdPath );
							system( s_CmdPath );
						}
					}
				}
				Win[0]->Reload_Entry();
				Win[1]->Reload_Entry();
				delete( s_CmdPath );
				break;
			case 'c':
				s_CmdPath = new char[ 4096 ];
				i_Flag = 0;
				for ( i_Cnt = 0 ; i_Cnt < Win[i_CsrWin]->GetItemNum() ; i_Cnt++ )	{
					if ( ( Win[i_CsrWin]->GetFlag(i_Cnt) & 1 ) == 1 )	{
						i_Flag = 1;
					}
				}
				if ( i_Flag == 0 )	{
					sprintf( s_CmdPath, "cp -r '%s' '%s'",
							Win[i_CsrWin]->GetFileName(),
							Win[i_CsrWin^1]->Entry->s_DirPath );
					Debug( s_CmdPath );
					system( s_CmdPath );
				}	else	{
					for ( i_Cnt = 0 ; i_Cnt < Win[i_CsrWin]->GetItemNum() ; i_Cnt++ )	{
						if ( ( Win[i_CsrWin]->GetFlag(i_Cnt) & 1 ) == 1 )	{
							sprintf( s_CmdPath, "cp -r '%s' '%s'",
									Win[i_CsrWin]->GetFileName(i_Cnt),
									Win[i_CsrWin^1]->Entry->s_DirPath );
							Debug( s_CmdPath );
							system( s_CmdPath );
						}
					}
				}
				Win[0]->Reload_Entry();
				Win[1]->Reload_Entry();
				delete( s_CmdPath );
				break;
			case 'e':
				s_CmdPath = new char[ 4096 ];
				move( 0, 0 );
				clear();
				refresh();
				sprintf( s_CmdPath, "vim '%s'", Win[i_CsrWin]->GetFileName() );
				system( s_CmdPath );
				delete( s_CmdPath );
				clear();
				Win[0]->Reload_Entry();
				Win[1]->Reload_Entry();
				break;
			case 8:		// BS
				Win[i_CsrWin]->ChDir((char*)"..");
				Win[i_CsrWin]->i_CsrPtr = 0;
				Win[0]->Reload_Entry();
				Win[1]->Reload_Entry();
				break;
			case 18:	// CTRL+R
				clear();
				refresh();
				break;
			case 0x1b:	// ESC
				Win[0]->Reload_Entry();
				Win[1]->Reload_Entry();
				break;
			case 'i':
				Win[i_CsrWin]->ChDir( Win[i_CsrWin]->GetFileName() );
				break;
			case 0x0a:
				if ( S_ISDIR( Win[i_CsrWin]->GetFileStat() ) )	{
					if ( strcmp( Win[i_CsrWin]->GetFileName(), ".." ) == 0 )	{
						Win[i_CsrWin]->ChDir((char*)"..");
					}	else	{
						Win[i_CsrWin]->ChDir( Win[i_CsrWin]->GetFileName() );
					}
					break;
				}	else	{
					i_Ch = 'v';
				}
			case 'v':
				s_CmdPath = new char[ 4096 ];
				move( 0, 0 );
				clear();
				refresh();
				sprintf( s_CmdPath, "less '%s'", Win[i_CsrWin]->GetFileName() );
				system( s_CmdPath );
				delete( s_CmdPath );
				clear();
				break;
			case '!':
				s_CmdPath = new char[ 4096 ];
				move( 0, 0 );
				clear();
				sprintf( s_CmdPath, "tcsh" );
				system( s_CmdPath );
				delete( s_CmdPath );
				Win[0]->Reload_Entry();
				Win[1]->Reload_Entry();
				clear();
				refresh();
				break;
			case '~':
				Win[i_CsrWin]->ChDir( getenv( "HOME" ) );
				Win[i_CsrWin]->Reload_Entry();
				Win[i_CsrWin]->Refresh();
				refresh();
				break;
			case 'T':
				Win[i_CsrWin]->i_CsrPtr = 0;
				Win[i_CsrWin]->CsrUp();
				Win[i_CsrWin]->Refresh();
				refresh();
				break;
			case 'B':
				Win[i_CsrWin]->i_CsrPtr = Win[i_CsrWin]->GetItemNum()-1;
				Win[i_CsrWin]->CsrDown();
				Win[i_CsrWin]->Refresh();
				refresh();
				break;
		}
		chdir( Win[i_CsrWin]->Entry->s_DirPath );
	}

//	strcpy( s_WorkBuf, Win[0]->Entry->s_DirPath );
//	i_Res = setenv( "FADIR1", s_WorkBuf, 1 );
//	strcpy( s_WorkBuf, Win[1]->Entry->s_DirPath );
//	i_Res = setenv( "FADIR2", s_WorkBuf, 1 );

	delete( Win[1] );
	delete( Win[0] );

	delete( s_StartTime_CurrentDir );
	delete( s_WorkBuf2 );
	delete( s_WorkBuf  );
	endwin();

}
