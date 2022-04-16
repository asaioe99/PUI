# FAQ

## DynamoRioについて

### CreateWindowEXをdrwrap_replace()で置き換える場合について
以下の様な場合を考えます。

```
if (_stricmp(module_name, "USER32.dll") == 0) {
	to_wrap = (app_pc)dr_get_proc_address(info->handle, "CreateWindowEX");
	drwrap_replace(to_wrap, (app_pc)CreateWindowEX_interceptor, NULL);
}

static HWND
CreateWindowEX_interceptor(
  DWORD     dwExStyle,
  LPCSTR    lpClassName,
  LPCSTR    lpWindowName,
  DWORD     dwStyle,
  int       X,
  int       Y,
  int       nWidth,
  int       nHeight,
  HWND      hWndParent,
  HMENU     hMenu,
  HINSTANCE hInstance,
  LPVOID    lpParam)
{
	if (lpClassNameはTOOLTIPS) return 適当な戻り値;
  return CreateWindowEX(
    DWORD     dwExStyle,
    LPCSTR    lpClassName,
    LPCSTR    lpWindowName,
    DWORD     dwStyle,
    int       X,
    int       Y,
    int       nWidth,
    int       nHeight,
    HWND      hWndParent,
    HMENU     hMenu,
    HINSTANCE hInstance,
    LPVOID    lpParam);
}
```
#### 原因
省略

#### 解決方法
不明

この様な場合、CreateWindowEX_interceptor内部のCreateWindowEX呼び出しに対しても、drwrap_replaceが影響してしまい、再帰的無限ループに陥りそうである。

## WinAFLについて
### trimに時間が掛かる
winAFL実行開始からbitflip戦略を開始するまでの間に、先だってtrimが実施される。この際、10分ほど時間を要することがあった。この時のexec speedは1/sec未満である。終了後のexec speedは正常である。

#### 原因
不明

#### 解決方法
しばらく待つ。

### bit flipsから先に進まない
単純に、seedとしてinputディレクトリに用意したファイルが大き過ぎる。なるべく小さいファイルを用意すべきで、例えば画像データのパーサをFuzzingするのであれば、専用のコーパスを利用するべきである。

#### 原因
bit flips戦略の特性上、処理量はファイルサイズに比例して大きくなる。

#### 解決方法
seedファイルのサイズを小さくする。

### 実行開始後しばらくしてもlast new pathが更新されない
WinAFLを実行してから数分以内にパスが通らない場合、恐らく解析対象のアプリケーションが正しく動作せず、入力ファイルを読み込むことができていないと考えられる。また、別に考えられる理由としては、デフォルトのメモリ制限```-m```が厳し過ぎるため、プログラムの割り当てに失敗して終了していることである。他にも、入力ファイルが最低限のファイルフォーマットに適合しておらず、基本的なヘッダチェックで弾かれている可能性も考えられる。

#### 原因
- 目的関数の設定誤り
- メモリ制限が強すぎる
- 入力ファイルが不適切

#### 解決方法
- 目的関数の再考
- メモリ制限の緩和
- 入力ファイル（seed）の再検討

### 途中から実行速度がとても遅くなった
exec speedが```-t```で指定した時間の逆数になっていないか確認せよ。もしそうであれば、毎回hangが発生していると思われる。

#### 原因
入力ファイルがtime out（又はcrash）を引き起こしている。

#### 解決方法
- ```-t```の指定時間を短くする
- 速度低下が終わるのを待つ

### 気が付いたら動作が止まっていてHDDの容量がなくなっていた
対象のアプリケーションが、一時的に隠しファイル（フォルダ）としてデータを生成しているのであれば、正常終了せずにそのまま削除されていない可能性がある。

#### 原因
アプリケーションの仕様

#### 解決方法
以下の様なbatファイルを実行すると良い。

```
:top
timeout 300

for /F %%a in ('dir /AD /B /W 202203*') do rd /S /Q %%a

goto top
```

### crashesディレクトリ内のファイル名は何か？
afl_fuzz.cの3677行以降が参考になる。以下は一覧である。

- EXCEPTION_ACCESS_VIOLATION
- EXCEPTION_ILLEGAL_INSTRUCTION
- EXCEPTION_PRIV_INSTRUCTION
- EXCEPTION_INT_DIVIDE_BY_ZERO
- STATUS_HEAP_CORRUPTION
- EXCEPTION_STACK_OVERFLOW
- STATUS_STACK_BUFFER_OVERRUN
- STATUS_FATAL_APP_EXIT
- EXCEPTION_NAME_NOT_AVAILABLE

### 途中で実行速度がとても遅くなった
もしも速度低下と同時にcrashesが増加しているのなら、それは各.cur_inputファイルが実行毎にcrashを引き起こしている可能性が高い。

#### 原因
crashの連続発生

#### 解決方法
正直なところどうしようもない。筆者の環境では、実行待ちのqueueが、どうbitflipしてもcrashを引き起こすように変異しており、丸二日間一秒毎の実行回数が１未満だった。最終的に、bitflip等の全ての戦略を終わらせるのに数週間は少なくとも必要であると見積もられたので、Fuzzingを終了した。

### dll全体にブレークポイントを設定したい
解析対象のアプリケーションが独自のdllを使用している場合、取り敢えずライブラリ単位で呼び出しを監視したいことがある筈だ。

#### 解決方法
```bm name_of_dll!*```により一括して設定できる。あくまでも個々の関数についてブレークポイントを設定していることに注意すべきである。

```
0:000> bm KERNEL32!*
  2: 7577b56f          @!"KERNEL32!RtlWideCharArrayCopyStringWorker"
  3: 757665b4          @!"KERNEL32!RtlStringCopyWideCharArrayWorker"
  4: 757324c0          @!"KERNEL32!TermsrvDeleteKey"
  5: 7572dad5          @!"KERNEL32!RtlStringCchCopyW"
  6: 75729d40          @!"KERNEL32!QueryInformationJobObject"
  7: 75729192          @!"KERNEL32!AslPathIsTemporaryInternetFile"
  8: 75735066          @!"KERNEL32!Internal_InvokeSwitchCallbacksOnINIT"
  
  省略
  
2531: 757288a0          @!"KERNEL32!CreateFileMappingWStub"
2532: 7575e8f6          @!"KERNEL32!SetUserGeoNameAndIdHelper"
2533: 7576bde0          @!"KERNEL32!SetMailslotInfo"
2534: 7577c6fb          @!"KERNEL32!AslFileMappingDelete"
breakpoint 2248 redefined
2248: 757457d0          @!"KERNEL32!UTUnRegister"
2535: 75765e30          @!"KERNEL32!AssignProcessToJobObject"
2536: 75733ab0          @!"KERNEL32!DeleteFileA"
2537: 75780bcb          @!"KERNEL32!SdbGetTagDataSize"
2538: 75734c90          @!"KERNEL32!BasepGetAppCompatData"
```

### Intel PTを使って高速にFuzzingしたい
Intel PTに対応したCPUであれば、これを利用して高速に（必ずしもDynamoRioよりも高速ではない）Fuzzingすることも可能である。

#### 解決方法
64bit版としてWinAFLをビルドし、その際に、

```
mkdir build64
cd build64
cmake -G"Visual Studio 16 2019" -A x64 .. -DDynamoRIO_DIR=..\path\to\DynamoRIO\cmake -DINTELPT=1
cmake --build . --config Release
```

とする。

また、実行時には、```-D```でDynamoRioを指定するのではなく、単に```-P```とだけ付ければ良い。

### 効率良く脆弱性を発見したい
様々な方法があるが、coverageの測定法で改善することがある。

#### 解決方法
実行時コマンドに、```-covtype edge```を付け、デフォルトの``` basic block```から変更する。

```:例
afl-fuzz.exe -i input -o C:\winafl_for_jwc\build32\bin\Release\JWW\output -t 10000 -D C:\DynamoRIO8.0.18460\bin32 -- -coverage_module common_lib.dll -coverage_module Jw_win.exe -target_module Jw_win.exe -target_offset 0x0283448 -fuzz_iterations 5000 -nargs 2 -covtype edge -call_convention thiscall -- C:\winafl_for_jwc\build32\bin\Release\JWW\Jw_win.exe @@
```

### エラーが出た１
以下の様なエラーが表示された。
![IMG_4943](https://user-images.githubusercontent.com/77034428/163205399-bf04e030-8bd6-4f0d-b2b3-c9d90ffec954.jpg)

#### 原因
WinAFLをビルドした際に指定したDynamoRioと、WinAFL実行時に```-D```で指定したDynamoRioのバージョンが異なる。

#### 解決方法
WinAFL実行時に指定するDynamoRioを、ビルド時のものと一致させる。

## WnDBGについて
### 使い方が分からない。
皆最初は同じです。

#### 原因
努力不足

#### 解決方法
以下を参照

http://windbg.info/download/doc/pdf/WinDbg_A_to_Z_mono_JP.pdf
