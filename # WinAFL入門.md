# 業務のためのFuzzing入門
本解説は、業務のためにFuzzingについて学ぶ必要のある人のために、取り急ぎ最低限の知識を身に着けるためのものである。その中でも特に、管理者と実務担当者を対象としており、実務担当者は本解説の各タイトルに〇印がある部分を、そして管理者は△印のある部分を拾い読みすれば良い。

この解説だけでは完結しない知識もあるので、その部分については参考文献を載せているので参照して頂きたい。また、管理者については実際にFuzzingを実施する訳ではないので、主に必要な環境や整えておくべき人材、更に機材や人材を評価する観点について説明する。しかしながら、管理者が適切に原理や一般論としてのFuzzingを理解していないと、本来得られるはずであるパフォーマンスが発揮されないばかりか、実務担当者の不平不満や怠慢を許す恐れがある。そのため、余力があれば実務担当者と同様に、実際にFuzzingを実施する体験をして欲しい。

本解説では、実務担当者の業務手順習得のために、実在するアプリケーション（JWCAD：CAD用のフリーソフト）に対してFuzzingを行う。そして、そのために必要な環境構築についても一通り解説する。そのため、ある程度の関連知識の不足があったとしても、管理者は実務担当者と一緒にFuzzingを体験できるすることができる。一度でも良いから実際の業務の流れを体験することで、様々な良い効果があることは間違いない。是非検討すべきである。

# Fuzzingの原理〇△
Fuzzingを行う目的は、対象のアプリケーションから脆弱性を自動的に検出することである。ここで言う脆弱性とは、不正なメモリアクセスや無限ループ（ hang）を誘起するような入力（以下「不正な入力」という）として発見される。

つまり、今回体験するJWCADに対するFuzzingの場合、JWCADに対して様々なファイルを読み込ませ、その中からJWCADに対して不正なメモリアクセスや無限ループを引き起こすようなファイルを発見するのである。

## 不正な入力を発見する方法〇
恐らく疑問を持った方も多いだろうが、不正な入力を発見する方法は一見して明らかではない。例えば、総当たりで全てのbitパターンを調査する方法が思いつく。この方法であれば、原理的には全ての不正な入力を発見できそうである。しかしながら、現実的な時間ではこの計算を終えることができないので、より効率的な方法を考える必要がある。

### 正規の入力を壊す〇
最初に思いつく方法は、元々正常な読み込みのできるファイルや入力を少しだけ変更する方法である。これには一応の理屈があって、

- いきなりランダムな入力を使うと、parserに弾かれる
- bit反転や延長で、入力値を気軽に変化させられる

というものである。

実際、これは悪くない戦略である。例えば、入力データ内のある部分が、その入力データのサイズを表していた場合、その部分の一部のbitを反転させると、本来のファイルサイズとは異なる値になってしまう。その場合、対象のアプリケーションに対策がさせていなければ、本来のデータサイズを超えた領域までアクセスしてデータを読み取ろうとしてしまうことが一応は予想される。

その結果、不正なメモリアクセスを行ってしまうかも知れない。また、入力された値に応じて、読み込みではなく書き込みを行うのであれば、特定のメモリ領域に対して意図した値を書き込めるかも知れない。そうすれば、何らかの悪意のあるコードを実行できるかも知れないのである。

上手く本来の入力の一部だけを壊すことにより、parserの目を誤魔化しつつも不正な操作を行える可能性がある。そして実際に、この戦略に似たものがWinAFLによるFuzzingで```bit flips```や```byte flips```として採用されている。

### メモリに明かりを灯す〇
他にも戦略がある。例えば、もしもメモリ（厳密にはtextセクション）のどの部分を実行しているか、その実行している箇所が光ったとしよう。そして更に、実行した部分はそのまま光り続けたとする。

その様な場合、プログラムを実行した場合、entryポイントから順に明かりが灯り、そして条件分岐によって実行箇所が飛んだ場合は、その先から同じように明かりが灯ることになる。最終的にプログラムが終了するまでに実行された部分は全て明かりが灯るが、例えば条件分岐により飛ばなかった部分は暗いままである。

これが何を意味しているかはそれほど明らかではない。しかし、もし本来コードとして実行されることを想定しない領域（データ領域）に明かりが灯ったら面白いはずである。つまり、入力したデータの何らかの作用により、本来想定されていない動作が生じた可能性が高い。

ここで、何らかの方法によってメモリに明かりを灯すことができれば、メモリの明るい部分の範囲（covarage：カバレッジ）の広さにより、脆弱性を生じさせ易さを計測することができそうである。

### 実際の方法〇
実際にはメモリに明かりを灯すことは困難であるので、より簡単な方法を採用している。例えば、我々が観測したいのは実行されたことのあるメモリの範囲であり、それを調べるだけであれば、条件分岐の部分だけを観測しておけばよい。つまり、トンネルの入り口と出口だけを監視していれば、トンネルの内部まで車を追わなくても良いのである。

そこで、実際の方法としては、このトンネルの入り口に相当する部分に対して、そこを通過したことを観測する特殊なコードを仕組んでプログラムを実行している。この特殊なコードというのは単なるジャンプ命令であり、トンネルの入り口で一度別の部分に車を誘導し、そこで車両が通過したことを記録した上で、再度トンネルの入り口に戻しているだけである。

これは例え話であるが、実際の場面ではバイナリ計装と呼ばれる考え方である。そして、ソースコードを書き換えて計装（instrumentation：インストラメンテイション）する場合と、実行中のプログラムに対して計装する方法があり、順に静的/動的バイナリ計装と呼ぶ。

そして、計装によりカバレッジを大きくするように入力を変化させることで、不正な入力を発見することができる。本解説では、カバレッジ拡大を目指す上で、どの様に入力を変化させるかについては説明しないが、念のために参考文献を示す。

参考：探しておきます

### 必要な時間〇△
これまでの説明で、不正な入力を発見するための考え方について説明した。問題は、どのくらいの時間をかけてFuzzingを行えば脆弱性を検出できるかである。結論から言えば、必要な時間は予測不可能である。解析対象のアプリケーションが一切脆弱性の存在しない完璧なものであれば、当然無限に時間をかけても発見することはできない。反対に脆弱性の塊の様なアプリケーションであれば、Fuzzing開始と同時に堰を切ったかの様に脆弱性が発見される。そしてこれらは、解析に利用するコンピュータの性能に大きく依存する。

そのため、どれくらい時間が必要かを見積もることよりも、どれくらいの時間で脆弱性が発見されたかで判断すべきである。つまり、高性能なコンピュータによる１ヶ月間ものFuzzingにより、一つも脆弱性が発見されなかったのであれば、それは相当に堅牢なアプリケーションであり、これを攻撃する者達も少なくともこれと同じ以上の解析費用を必要とする。

また、攻撃者としての視点に立てば、対象のアプリケーションを自由に選ぶことのできない状況であれば、如何なるコストを支払ってでもFuzzingを継続し、脆弱性を何としてでも発見しなければならない。結果的に発見できない可能性もある訳で、もし時間的制約があるのであれば、一定の基準を定め、その時間内に成果を出せなければ打ち切る必要がある。

### 必要なコンピュータの性能〇△
今までの話から単純に論じれば、コンピュータの性能が倍になれば、同じ時間で発見できる脆弱性の数も倍になりそうに思える。これはある程度は事実であるが、注意点もある。

ここで押さえておきたいことは、発見した不正な入力の数を単純に評価してはならないということである。重要なのは種類である。つまり、プログラム中の同じ脆弱性に対して作用する不正な入力を作るだけなら、実はいくらでも数を増やすことができてしまうのであり、本来評価すべきなのはプログラム中に存在する脆弱性をいくつ発見したかである。この事実は必ず押さえておく必要がある。そして発見可能な脆弱性の種類は、使用するFuzzerや解析者の能力にも依存している。そのため、厳密な議論は困難である。

さて、以上を理解した上で、それでもコンピュータの性能が重要であることを強調しよう。そして、以下の様な点に注意して導入するコンピュータを決定すべきである。

- CPUのコア数が多い
- メモリのサイズが大きい
- 高速に読み書きできる不揮発性記憶媒体（SSD）を使用している
- 堅牢で信頼性が高い
- 突然の停電等にも対応できる電源を有している

以上の条件について、参考として現時点(2022/3/12)で快適なFuzzingを実施する上で標準的な環境を以下に示す。

#### CPU
最も重要な要素であるため、優先してリソースを割くべきである。以下は、優れた性能であると考えられる具体的な製品の紹介である。

- Intel Core i9-12900K

  16コア24スレッドであるため、WinAFLであれば最大24プロセスの並列処理が可能である。また、シングルコア当たりの性能が高いため、優れたパフォーマンスを発揮できる。また、Intel PTという機能に対応しているため、特定の条件のみではあるが、他社同等製品のCPUよりも１桁以上高速にFuzzingを実施可能な場合がある。

- AMD Ryzen Threadripper PRO 3995WX

  64コア128スレッドであり、現時点では最速のFuzzingが実施可能であると考えられる。

#### メモリ
十分な容量が必要であるが、特に重視すべき点は読み書きの速度である。現時点ではDDR5 SDRAMという規格が最速であり、この規格のメモリを使用することが望ましい。容量は、多ければ多いほど良く、少なくとも32GByteは必要である。

#### ハードディスク
容量よりも高速な読み書き性能が重視される。特に、Fuzzing対象のアプリケーションがハードディスクにアクセスするような動作をする場合、別に特別な対策を採用しない場合、この読み書き速度に律速される。そのため、現時点で最速の性能であるPCIe 4.0規格のSSDを使用することが推奨される。

#### 電源等
突発的な停電により機材が停止すると、それまでに実施していたFuzzingの結果が失われてしまう可能性がある。そのため、複数の冗長性による堅牢な電源構成を構築すべきである。そのため、端末専用のUPSは必須であり、併せて施設そのものの電源についても、商用とは別に発動発電機とUPSを導入することが理想である。

#### その他
以下は個別の要考慮事項である。重要なものも含まれているので、必ず目を通すべきである。

- アンチウイルスソフトについて

  アンチウイルスソフトは、Fuzzingと同様の技術を用いているため、同時に用いることはできない。そのため、Fuzzing端末についてはOS標準に機能を除き、一切のアンチウイルス製品を導入してはならない。

- 各種解析ツール

  Fuzzingを実施する際には、先だって対象のアプリケーションを解析する必要がある。そのため、WinDBGやGhidra等のツールが必要になる。ここで重要なのは、これらのツールは必ずしも商用の製品は必要ではなく、全てOSSのもので十分に要件を満たすが、一方で、インターネット接続環境が制約なく整備されていない場合、これらのOSSを満足に利用できない。そのため、以降で示す環境構築時の手順を熟読し、これらの環境を不便なく構築できるようにしなければならない。

- 端末の数について

  最小構成であっても、Fuzzing用の端末と解析用の端末で各１台は必要である。解析用の端末は、Fuzzing用とは異なり最高性能を目指す必要はない。また、Fuzzing用の端末については、複数の端末を連携・分散させることもできるため、より高速なFuzzingが求められる場合は複数の端末を導入し、ネットワークを構成することも考えるべきだる。残念ながらこの様な運用方法に関する実用的な経験や技術文献がないため、本解説では触れることができない。

### 必要な人材△
組織としてFuzzingを行う際に注意したいことは、必要な機材に関する特性だけではなく、人材についても多くある。以下は考慮すべき事項である。

- プログラミングの経験がある（具体的な成果物を示すことができる）
- 脆弱性の原理について知っている（専門用語を使わないで説明できる）
- 自主的に学習を進められる性格である（技術進歩が早いため、常に勉強が必要）
- 知識・技術を周囲に普及できる（効率化には情報共有が必須である）
- コンピュータ本体に関する知識と技術がある（環境構築や維持運用に必要）
- 英語や中国語を始めとした外国語の読解力（技術情報の大半は外国語）

他の技術とも共通する要素は少なくないが、Fuzzingに関する技術情報は広く出回ってはいないため、体系的な入門書や解説書は存在していない。そのため、自発的に情報を収集し、新たな技術を学習し、そして発見した技術や情報を組織に還元する能力が必要である。そして、ここで示したような能力を発揮した人間を評価すべきである。

## 不正な入力を発見したら〇
今までに説明した様々な方法により、メモリアクセス違反や無限ループを引き起こすような入力を発見した場合、次に行うのは、入力したデータのどの部分がプログラムのどの部分に作用して不正な動作を引き起こしたかを解析することである。

この解析を行うのに必要な技術は以下の通りである。

- C/C++言語の実用的な技術
- 静的解析の技術
- 動的解析の技術
- アセンブリの実用的な技術
- アーキテクチャに関する実用的な知識
- OSに関する実用的な知識


これらの知識及び技術を用いることで、発見した不正な入力を元に、アプリケーションのどの部分に脆弱性が存在し、そしてその仕組みについて明らかにすることになる。

その過程で、脆弱性の緩和策（修正方法や軽減方法）を考えることも必要になるだろうし、反対にその脆弱性を利用した攻撃方法を考える必要もあるだろう。

### 脆弱性のメカニズム
解説は後で。参考文献は以下

参考；
https://www.ipa.go.jp/security/awareness/vendor/programmingv2/cc01.html

### 脆弱性の修正方法
後で追加。```_s```の関数について簡単に紹介したい。あとは開発環境をそもそも変えるとか？

### 脆弱性の攻撃方法
解説の範囲が逸脱するけど、追加したい。エクスプロイトの紹介だけで済ませる？

# WinAFL入門
前置きが長くなったが、これからは実際にFuzzingを実施し、脆弱性を発見する。本解説では脆弱性の修正方法や攻撃方法については取り扱わない。したがって、本解説と同様の方法で別のアプリケーションに大してFuzzingを行い、何らかの脆弱性を発見した場合は、そのアプリケーションの開発者と直接連絡ができる場合は相談することを勧める。

## 必要なものリスト
- WinAFL

   https://github.com/googleprojectzero/winafl
   
   Fuzzingの対象によっては、自分でソースコードを修正して都度ビルドする必要がある。

- DynamoRIO-Windows-8.0.18460

   https://github.com/DynamoRIO/dynamorio/releases/tag/cronbuild-8.0.18460

   DynamoRioはビルドせずにそのままbin32/64を使用

- Visual Studio Community 2017 (version 15.9)

   https://my.visualstudio.com/Downloads?q=visual%20studio%202017&wt.mc_id=o~msft~vscom~older-downloads

   WinAFLのビルドに使用

- cmake-3.23.0-rc3-windows-x86_64

   https://cmake.org/download/
   
   WinAFLのビルドに使用

- git for windows version 2.35.1.2

  https://gitforwindows.org/

  WinAFLのダウンロードに使用

- JWCAD　(ver8.24a)

   https://www.jwcad.net/download.htm

   今回の解析対象

- Ghidra 10.1.2

   https://github.com/NationalSecurityAgency/ghidra/releases/tag/Ghidra_10.1.2_build

   JWCADを解析し、```target_offset```を選定するために使用

- WinDbg Preview

   https://www.microsoft.com/ja-jp/p/windbg/9pgjgd53tn86?SilentAuth=1&wa=wsignin1.0&rtc=1&activetab=pivot:overviewtab

   JWCADを解析し、```target_offset```を選定するために使用

## DynamoRioの準備
DynamoRioは、動的バイナリ計装に関する機能を提供するライブラリである。動的バイナリ計装はGrey Box Fuzzingでは必須の技術である。ここまでの解説により、理解に必要な知識が一通り揃ったので、Fuzzingの種類について触れた上で、DynamoRioの導入方法について説明する。

### White Box Fuzzing
White Box Fuzzingは、解析対象のアプリケーションのソースコードを自由に利用可能な場合（ソースコードがない場合でも一応は可能だが、問題が多い。詳細は後述）に用いる方法である。WinAFLのフォーク元であるAFL（Linuxアプリケーションに対してWhite Box Fuzzingを行うツール）では、専用のコンパイラにより、解析対象のアプリケーションのソースコードをコンパイルする際に静的バイナリ解析用のコードを仕込むことによりカバレッジを計測している。

### Grey Box Fuzzing
本解説で説明するWinAFLでは、DynamoRioにより動的バイナリ計装を行い、解析対象のプリケーションを動作させながら、カバレッジを測定する。実行中のアプリケーションの動作を動的に変更しながらカバレッジを計測するため、解析に要する速度は静的バイナリ計装、つまりWhite Box Fuzzingよりも低速になる。

しかしながら、解析対象のアプリケーションのソースコードが入手できる場合は限定的であり、実際の業務の上ではGrey Box Fuzzingが主体となると考えられる。そのため、本解説ではGrey Box Fuzzingのみを扱う。

また、これとは別にBlack Box Fuzzingという手法も存在する。これは、対象のアプリケーションそのもの（実行ファイル等）が入手できない場合に行うFuzzingのことを指すが、筆者にはこの方法に関する十分な経験がないため、説明はできない。

### 動的バイナリ計装と静的バイナリ計装の比較
ここで、動的バイナリ計装（DBI）と静的バイナリ計装(SBI)の特徴を比較する。

| DBI  | SBI  |
| ---- | ---- |
|-低速|+高速|
|-解析環境の共有が困難|+解析環境の共有が容易|
|+解析対象のライブラリを考慮しない|-解析対象のライブラリも計装する必要がある|
|+解析対象の動作の一部のみを解析可能|-解析対象の動作全般を解析しなければならない|

### 静的バイナリ計装についての補足
静的バイナリ計装を、解析対象のソースコードが入手できない場合に実行した場合の特徴について解説する。

この様な条件でも解析は可能で、その場合は解析対象を逆アセンブルし、バイナリレベルで計装コードを追加することになる。研究用のツールとして、PEBILやDyninstと呼ばれるものが利用可能だが、発展途中である。ここでは詳細についてはこれ以上触れないが、動的バイナリ計装との比較を下に示す。

| DBI  | SBI  |
| ---- | ---- |
|+逆アセンブルが不要|-逆アセンブルが必要でエラーが起こりやすい|
|+バイナリの書き換えが不要|-バイナリの書き換えでエラーが起こりやすい|
|+pdbファイルの様なシンボル情報が不要|-安定動作のためにはシンボルが必要|

### DynamoRioのダウンロード
今回の解説で使用するDynamoRioのバージョンは、```DynamoRIO-Windows-8.0.18460```である。ダウンロード先は以下の通りである。

URL：
https://github.com/DynamoRIO/dynamorio/releases/tag/cronbuild-8.0.18460

```DynamoRIO-Windows-8.0.18460```フォルダは、自由に名前を変更しても良い。WinAFLの実行時には、フォルダ内のbinフォルダのパスをコマンドに指定しなければならないので、なるべく短い名前にすべきである。しかしながら、WinAFLのバージョンや解析対象のアプリケーションとの相性によっては、複数のDynamoRioのバージョンを使い分ける必要があるため、使用するバージョンは明示すべきである。そのため、本解説では```DynamoRIO8.0.18460```という名称に変更する。

さて、ダウンロードが終わったらフォルダの中身を確認してほしい。以下の様になっている筈である。

- bin32

  32bit用のビルド済ファイル
- bin64

  64bit用のビルド済ファイル
- cmake
- docs
- drmemory
- dynamorio

  技術資料が含まれている
- ext
- include
- lib32
- lib64
- logs
- samples
- ACKNOWLEDGEMENTS
- Licence.txt
- README

基本的にはこのまま使用するのでビルドする必要はないが、WinAFLが上手く動作しない場合は、解決方法の一つとしてDynamoRioのFuzzing環境下でのビルドがある。そのため、以下の手順は問題が発生した場合のみ参照すれば良い。

### DynamoRioのビルド〇
後で追加します。

### DynamoRioの関数紹介〇
解析対象のアプリケーションによっては、動作中にメッセージボックスが表示される場合がある。この場合、メッセージボックスのボタンを押下するまで動作が停止する場合もあるため、Fuzzingを実行する上では解決しなければならない。

一つの方法として、VBSによりこれらの操作を自動化する方法があるそうだが、筆者は試したことがない。より洗練された方法としてはDynamoRioにより、メッセージボックスの表示を行うAPI（MessageBoxAやMessageBoxW）をフックして、別に解析者が用意したダミーの処理とすり替える方法がある。

以下は、フックによる処理のスキップの具体例である。尚、本例はJWCADのFuzzingでも用いるため、機能の具体的な実装方法はそちらでも示す。

さて、今回使用するDynamoRioの関数は２つあり、それぞれ順に説明する。

補足：
https://dynamorio.org/group__drwrap.html#ga488a6566cd760a3919bdd2f49a6d672f に解説があるのでそちらを参考にしても良いが、初めての場合は難しいと思う。

#### ```dr_get_proc_address```の詳細
```
DR_API generic_func_t dr_get_proc_address(module_handle_t lib,
		                                  const char *    name) 	
```
この関数は、第１引数に対してdllのベースアドレスを指定し、第２引数に関数名を指定することで、戻り値として当該関数のエントリーポイント（先頭のアドレス）を取得できる。

#### ```drwrap_replace```の詳細
```
DR_EXPORT bool drwrap_replace(app_pc original,
		                      app_pc replacement,
		                      bool   override) 	
```
この関数は、第１引数に指定したアドレスの関数を、第２引数に指定したアドレスの関数で置き換えるものである。更に第３引数で```true```を指定した場合、先に第１引数で指定したアドレスの関数が別き換えられるように別で設定されていた際に、これに優先して置き換えることができる。```NULL```の場合はその逆である。

#### 実際の使用例
先に紹介した両関数を組み合わせた応用例について説明する。

```
if (_stricmp(module_name, "USER32.dll") == 0) {
	to_wrap = (app_pc)dr_get_proc_address(info->handle, MessageBoxW");
	drwrap_replace(to_wrap, (app_pc)Messageboxw_interceptor, NULL);
}
```
この例では、```module_name```が文字列```USER32.dll```と一致した場合、ポインタ```to_wrap```に対して```dr_get_proc_address```関数により取得した```MessageBoxW```関数のアドレスを代入している。その後、```drwrap_replace```関数の第１引数に```to_wrap```を指定し、```MessageBoxW```が呼び出された際にこれをフックして、代わりに準備した```Messageboxw_interceptor```関数とすり替えている。

## Visual Studio 2017の準備〇
Visual Studio 2017は、WinAFLのビルドに必要である。基本的には[こちら](https://my.visualstudio.com/Downloads?q=visual%20studio%202017&wt.mc_id=o~msft~vscom~older-downloads) からVisual Studio Community 2017(version 15.9)を選択してダウンロードすれば良い。インストーラを入手したら、あとは手順に従ってインストールを完了させれば良い。

注意点としては、スタンドアロン端末にインストールする場合は別の方法が必要であり、本来の方法よりも数倍は時間も手間も掛かる。問題が発生した際の対処にも同様に時間と手間を要し、しかも場合によってはスタンドアロン環境では解決不可能な場合もあるため、業務としてFuzzingを実施する場合は、この点について特に慎重に検討をすべきである。その上で、オフライン環境下でのVisual Studio のインストール方法について説明する。

### オフライン環境下でのインストール手順〇
まずは、インターネットに接続している環境下で、オフラインインストールファイルを作成する。

1. [Visual Studio 2017](https://my.visualstudio.com/Downloads?q=visual%20studio%202017&wt.mc_id=o~msft~vscom~older-downloads)のインストーラをダウンロード
2. インストーラを任意の空のフォルダに配置
3. 下記のコマンドを１行ずつ実行しダウンロード完了を待つ

```
mkdir in_vs2017
vs_community__xxxxxxxx.xxxxxxx.exe --layout in_vs2017 --lang ja-JP
```
4. ダウンロード端末にフォルダを移動する
5. フォルダ内の```vs_community__xxxxxxxx.xxxxxxx.exe```を実行

後は、通常の手順と同じである。

## cmakeの準備〇
cmakeはWinAFLのビルドで必要なツールである。[こちら](https://cmake.org/download/)から、```cmake-3.23.0-rc3-windows-x86_64.msi```をダウンロードし、インストールを始めれば良い。オフライン環境でも動作するため、Fuzzing端末がスタンドアロンであっても問題なくインストールできる。

## git for windowsの準備〇
後で書く

## WinAFLの準備〇
WinAFLの準備は大変である。まず前提として、Visual Studio 2017のインストールが必要なので、まだ済ませていないのであれば、先に説明した通りに実施して頂きたい。また、cmakeのインストールも忘れずに実施しなければならない。

### WinAFLのビルドの練習〇
本解説では、Cドライブ直下にWinAFLをインストールすると仮定して説明する。

まずは、スタートメニュー → すべてのアプリケーション → Visual Studio 2017 → x86 Native Tools Command Promptを開く。（Windowsキー押下後に、```x86```と入力すると候補に表示される）

その後、```x86 Native Tools Command Prompt```上で以下の様に実行する。

```
$ cd C:\
$ git clone --recursive https://github.com/googleprojectzero/winafl
$ cd winafl && mkdir build32 && cd build32
$ cmake -G"Visual Studio 15 2017" .. -DDynamoRIO_DIR=path_of_DynamoRIO_cmake
$ cmake --build . --config Release
```
```-DDynamoRIO_DIR=```以下にはDynamoRioのcmakeフォルダのパスを指定する。cmakeフォルダをshift + 右クリックで```パスのコピー```をクリックすれば簡単である。

もしも上記例と同様にgitが利用できない場合は、手動でWinAFLをダウンロードし、```git clone --recursive```により本来ダウンロードされるはずであった、```third_party```フォルダ内のファイルを全て手動で用意する必要がある。

手動での準備が終わったら、

```
$ cd C:\
$ cd winafl && mkdir build32 && cd build32
$ cmake -G"Visual Studio 15 2017" .. -DDynamoRIO_DIR=path_of_DynamoRIO_cmake
$ cmake --build . --config Release
```

として、当初の手順に合流すれば良い。

WinAFLのダウンロードは、[こちら](https://github.com/googleprojectzero/winafl)で、code → Download ZIP を選択すれば良い。

画像：winaflgit

念のために、上記コマンドの主要な手順を順に示すと、

```
C:\winafl\build32>cmake -G"Visual Studio 15 2017" .. -DDynamoRIO_DIR=C:\DynamoRIO8.0.18460\cmake
```

画像：cmake1

```
C:\winafl\build32>cmake --build . --config Release
```

画像：cmake2

となる。

無事にビルドが完了したら、build32フォルダ内に多数のファイル等が生成されている筈である。

今後の流れとしては、Fuzzing対象のアプリケーションに応じてWinAFLを改造し、その都度専用のFuzzerとしてビルドすることになる。そのため、今回説明したビルド手順には慣れておくべきである。


## JWCADの解析△〇
Fuzzingを開始する前に、解析対象のアプリケーションを調査する必要がある。基本的な考え方としては、どの様に対象のアプリケーションに入力を与えることができるか、である。つまり、アプリケーションに対してGUIの操作を要する方法でしか入力を与えられない場合は、何らかの方法でこの操作を自動化しなければならない。これは簡単ではなく、相応の経験と技術が必要で、Windowsアプリケーションの開発経験があれば理想である。

さて、今回の場合、JWCADは以下の様なコマンドによって読み込むファイルを指定して起動することができる。そのため、JWCAD本体を修正する必要は一切ないが、いくつかの問題からWinAFLに手を加える必要がある。

```
Jw_win.exe file.jww
```
ここで説明するのは、上記コマンドの通りにJWCADを操作した際に、何らかの人の手による操作が不要であるかを確認し、もしも何らかの問題があれば、どの様にそれを除去するかである。

### 拡張子
JWCADで対応している拡張子は、```JWC```、```JWW```、```DXF```、```SFC```、```P21```の５種類である。そこで、正規のjwwファイルの拡張子を別の非対応の拡張子jpgに変更して読み込ませてみる。すると、```JW_CADでは読み込めないファイルです```と表示された。

これにより１つの事実が判明した。つまり、JWCADは最初に拡張子でファイルの種類を判定しており、仮に正規のファイルであったとしても、拡張子が上記のいずれかでなければならないということであることが分かる。

更に実験してみると、jwwファイルの拡張子をp21に変更しても問題なく開くことができた。つまり、上記５種類の拡張子であれば取り敢えずファイルの読み込み自体は行われ、その過程で何らかの方法によりそれぞれのフォーマットを判定して読み込んでいる。

試しに、全く無関係なbmpファイルの拡張子をjwwに変更して読み込ませてみると、```このファイル形式には対応していません```と表示された。これにより、上記の５種類のいずれかのファイルであると判定されなかった場合は、それ以上読み込みを行わないことが分かる。

これらの事実から、WinAFLの生成する入力ファイルにはいずれかの拡張子を付けなければならないことが分かる。実は、WinAFLはFuzzingの過程で生成する入力ファイルには拡張子を省く仕様になっている。そこで、この部分について後ほど修正が必要である。

### メッセージボックス
実は既に拡張子とファイル読み込みの部分で判明しているが、入力されたファイルが正規のフォーマットではないと判断されると、メッセージボックスが表示され、動作が中断してしまう。その都度動作が停止してしまうと、Fuzzingを適切に実行できない。

そこで、MessageBox関数の呼び出しをフックすることで、別に用意した関数とすり替え、動作を継続されられるように、WinAFLを後ほど改造する。詳細についてはDynamoRioの説明である程度しているので、必要に応じて遡って参考にしてもらいたい。

## WinAFLの改造
JWCADの解析結果により、以下の２点の修正が必要であることが分かった。

- ファイル拡張子の追加
- メッセージボックスのすり替え

そこで、これらの修正方法を説明する。

### afl-fuzz.cの修正
修正箇所は```afl-fuzz.c```内の計３箇所である。ソースコード内を```.cur_input```で検索すれば良い。この```.cur_input```はWinAFLがFuzzing中に生成する、これから入力するファイルの名前である。そこで、今回は以下の様にソースコードを訂正する。

訂正前
```
  fn = alloc_printf("%s\\.cur_input", out_dir);
  if (unlink(fn) && errno != ENOENT) goto dir_cleanup_failed;
  ck_free(fn);
```
訂正後
```
  fn = alloc_printf("%s\\.cur_input.jww", out_dir);
  if (unlink(fn) && errno != ENOENT) goto dir_cleanup_failed;
  ck_free(fn);
```

５種類の拡張子のいずれかであれば問題ない。今回は```jww```拡張子にした。他の２箇所にいついても、同様に修正すれば良い。

### WinAFL.cの修正
最初に、```MessageBoxA```及び```MessageBoxW```と同じ引数と戻り値を持つ無害な関数を定義する。戻り値としては、常に「はい」を押下されたことになる様に、定数値を返すのみにする。

608行辺りに以下を追加する。
```
static int
messageboxa_interceptor(HWND hWnd, LPCSTR lpText, LPCSTR lpCaption, UINT uType) {
	return IDOK;
}

static int
messageboxw_interceptor(HWND hWnd, LPCWSTR lpText, LPCWSTR lpCaption, UINT uType) {
	return IDOK;
}
```

更に、```messageBox```の呼び出しをフックし、上記関数とすり替えるために、740行辺りに以下を追加する。
```
if (_stricmp(module_name, "USER32.dll") == 0) {
	to_wrap = (app_pc)dr_get_proc_address(info->handle, "MessageBoxW");
	drwrap_replace(to_wrap, (app_pc)messageboxw_interceptor, (bool)NULL);
	to_wrap = (app_pc)dr_get_proc_address(info->handle, "MessageBoxA");
	drwrap_replace(to_wrap, (app_pc)messageboxa_interceptor, (bool)NULL);
}
```
注意点としては、```USER32.dll```は大文字にする必要があることと、```dr_get_proc_address```及び```NULL```は、上記の通りに型変換をしないと、ビルド時に警告を受けることくらいだろう。

### WinAFLの再ビルド
ここまでの修正を完了させたら、再度ビルドを行う。手順としては既に紹介したが、念のために```build32```を消去する。また、今後新たにFuzzingを行うアプリケーションのために、都度これらの修正内容についての技術詳細をドキュメント化すべきであり、業務の属人化を防がなければならない。

### ビルドが終了したら
今までの作業で、Fuzzingに必要な準備の大半は終了した。残すところは```targrt_offset```の選定と、シード（seed：種）ファイルの準備である。どちらも必ずしも１つに絞ることはできないものであり、解析者の腕によりFuzzingそのもののパフォーマンスを大きく作用する重要な技術である。更に、状況に応じてカバレッジ計測対象についても考慮する必要もある。

いずれにせよ、解析者は広く関連する技術分野を理解していなければならないのであって、各分野の背景になる基本的な考え方や原理を蔑ろにはできない。反対に言えば、基礎となる以下の分野をしっかりと抑えていれば、短時間で優秀なFuzzing実務担当者になることができる。

- 計算機科学
- 低レイヤー
- 各種ファイルフォーマットに関する実用的な理解

特に、何らかのファイルフォーマットについて、相応な時間をかけて仕様を調べ、理解した経験があると理想的である。多くの場合、Fuzzingの結果発見された不正な入力は、対象アプリケーションの扱うファイルフォーマット上の些細な特徴に対し、これを処理するアプリケーションの不備を突くことによって不正な動作を引き起こすことになる。そのため、ファイルフォーマットを詳しく調査し、理解した経験は必ず活きる筈である。

## Fuzzing実施のための準備
いよいよFuzzingの実施に取り掛かる。ここからは、実施の上で必要な各種コマンドのオプションを正確に設定する方法について説明する。入念な事前の解析により、対象アプリケーションに併せたWinAFLの修正が済んでいれば、多くの場合、Fuzzingが適切に実行されない原因はオプションの指定方法に誤りがある。

### 各種オプション一覧
以下が使用するオプションの一覧であるが、今回の解析で使用するものはこの中の一部である。

コマンドは以下の様に３つの部分により構成されている。

```
afl-fuzz [afl options] -- [instrumentation options] -- target_cmd_line
```

翻訳や加筆は後でやる

```[afl options]```
```
-i dir        - テストケース用のinputディレクトリを指定
-o dir        - fuzzerが発見したファイルを格納するoutputディレクトリを指定
-t msec       - timeout for each run
-s            - deliver sample via shared memory
-D dir        - directory containing DynamoRIO binaries (drrun, drconfig)
-w path       - path to winafl.dll
-e            - expert mode to run WinAFL as a DynamoRIO tool
-P            - use Intel PT tracing mode
-Y            - enable the static instrumentation mode
-f file       - location read by the fuzzed program
-m limit      - memory limit for the target process
-p            - persist DynamoRIO cache across target process restarts
-c cpu        - the CPU to run the fuzzed program
-d            - quick & dirty mode (skips deterministic steps)
-n            - fuzz without instrumentation (dumb mode)
-x dir        - optional fuzzer dictionary
-I msec       - timeout for process initialization and first run
-T text       - text banner to show on the screen
-M \\ -S id   - distributed mode
-C            - crash exploration mode (the peruvian rabbit thing)
-l path       - a path to user-defined DLL for custom test cases processing
-A module     - a module identifying a unique process to attach to
```

#### [instrumentation options]
```
-covtype         - the type of coverage being recorded. Supported options are
                   bb (basic block, default) or edge.
-coverage_module - module for which to record coverage. Multiple module flags
                   are supported.
-target_module   - module which contains the target function to be fuzzed.
                   Either -target_method or -target_offset need to be
                   specified together with this option.
-target_method   - name of the method to fuzz in persistent mode. For this to
                   work either the method needs to be exported or the symbols
                   for target_module need to be available. Otherwise use
                   -target_offset instead.
-target_offset   - offset of the method to fuzz from the start of the module.
-fuzz_iterations - Maximum number of iterations for the target function to run
                   before restarting the target process.
-nargs           - Number of arguments the fuzzed method takes. This is used
                   to save/restore the arguments between runs.
-call_convention - The default calling convention is cdecl on 32-bit x86
                   platforms and Microsoft x64 for Visual Studio 64-bit
                   applications. Possible values:
                       * fastcall: fastcall
                       * ms64: Microsoft x64 (Visual Studio)
                       * stdcall: cdecl or stdcall
                       * thiscall: thiscall
-debug           - Debug mode. Does not try to connect to the server. Outputs
                   a log file containing loaded modules, opened files and
                   coverage information.
-logdir          - specifies in which directory the log file will be written
                   (only to be used with -debug).
-thread_coverage - If set, WinAFL will only collect coverage from a thread
                   that executed the target function
```

#### target_cmd_line
今回の場合は以下の例の通りになる。尚、```@@```は入力ファイル等のプレースホルダとなり、```.cor_input```がここに代入される。

```
C:\winafl_for_jwc\build32\bin\Release\JWW\Jw_win.exe @@
```

#### 実行コマンド例
結論から記せば、今回の解析では次の通りのコマンドを実行することになる。

```
afl-fuzz.exe -i input -o C:\winafl_for_jwc\build32\bin\Release\JWW\output -t 10000 -D C:\DynamoRIO8.0.18460\bin32 -- -coverage_module common_lib.dll -coverage_module Jw_win.exe -target_module Jw_win.exe -target_offset 0x0283448 -fuzz_iterations 5000 -nargs 2 -call_convention thiscall -- C:\winafl_for_jwc\build32\bin\Release\JWW\Jw_win.exe @@
```

#### フォルダ等の配置
Cドライブ直下にWinAFL及びDynamoRIO8.0.18460を配置する。また、JWCADのJWWフォルダ及びシードを入れたinputフォルダはともに```C:\winafl\build32\bin\Release```に配置する。更に、JWWフォルダ内には空のoutputファイルを作成する。

### -i
このオプションは、seedファイルを格納するinputフォルダを指定するものである。基本的には```afl-fuzz.exe```と同じ階層に準備すれば良い。内部に入れるseedファイルの選び方については後述する。

### -o
このオプションにより指定されたフォルダには、以下のフォルダとファイルが自動的に作成される。

#### フォルダ
- crashes
- drcache
- hangs
- ptmodules
- queue

#### ファイル
- .cur_input(.jww)
- fuzz_bitmap
- fuzzer_stats
- plot_data

重要なものについてのみ説明する。```crashes```フォルダ内には、メモリアクセス違反等でアプリケーションがクラッシュした場合の入力ファイルの内、ユニークなものが格納される。また、```hangs```フォルダ内には```-t```で指定した以上の時間、動作が中断した場合の入力ファイルを格納する。大抵の場合、それは無限ループを引き起こしてフリーズさせる入力ファイルである。

```.cur_input(.jww)```ファイルについては既に説明しているが、現在入力されているファイルである。これは、```queue```フォルダ内で読み込み待ちをしているファイルから順に呼び出されることになる。

### -t msec
指定した値が、各実行における制限時間となる。単位はミリ秒であり、通常はそれほど大きな値を指定する必要はない。今回は10,000 msec程度で十分である。

### -D dir
DynamoRioのbin32/64ディレクトリのパスを指定する。

### -coverage_module
カバレッジを計測する対象を指定する。今回の場合、```Jw_win.exe```は当然として、併せて```Jw_win.exe```が使用する```common_lib.dll```も追加で指定する。少なくとも前者のみを指定していれば良い。

### -target_module
Fuzzingの対象を指定する。当然、今回の場合は```Jw_win.exe```を指定する。

### -target_offset
対象アプリケーション内のサブルーチン（関数、メソッド）の先頭アドレスを指定する。この値の策定方法は簡単ではないため、別に詳しく説明する。今回の場合は、```0x0283448```を指定する。

### -fuzz_iterations
Fuzzingを高速に実行するために、WinAFLは```-target_offset```で指定した関数で一度スナップショット（レジスタやスタックの値を保存）を撮り、各実行毎にリセットをしてこの地点から実行することで、プロセスを実行毎に丸ごと最初から再起動しなくても良いように工夫されている。しかしながら、完璧にスナップショットを撮っている訳でないため、実行回数が増えるにつれてプロセス内部のメモリ等の中身が変化してしまい、やがて正常に動作しなくなる。

そこで、一定回数の実行毎に動作を最初からリセットさせることで、安定的に動作できるようにしている。```-fuzz_iterations```では、このリセットを実施するまでの回数を指定する。この値は小さい方が動作が安定し、大きくすることで動作を高速化できる。適切な値を事前に決定することは困難であり、解析対象のアプリケーションに依存する。今回は10,000回を採用する。

### -nargs
```-target_offset```で指定した関数の引数の数を指定する。Ghidra等で解析するのが早いが、デバッガを用いて逆アセンブリにより解析する方が確実である。

### -call_convention
```-target_offset```で指定した関数の呼び出し規則を指定する。関数は、一般的に呼び出し規則に応じて戻り値の渡し方等、動作が異なる。そこで、正しい呼び出し規則を指定しないと、スナップショットを元に繰り返し実行することができない。関数の呼び出し規則を調べる方法は多数あるが、Ghidraを使用するのが簡単で良い。各種関数呼び出し規則については、リバースエンジニアリングを行う上で必須の知識なので、この際にしっかりと学ぶべきである。尚、今回の場合は```thiscall```を指定する。

関数呼び出し規則の良い資料があったので、後で追記する。

## Fuzzingの実施
これまでに解説した通りに、```afl-fuzz```の実行コマンドは３つの部分で構成されている。各部分を適切に設定できたのなら、それらを```--```により結合する。今回の場合は以下の通りになる筈である。

```
afl-fuzz.exe -i input -o C:\winafl_for_jwc\build32\bin\Release\JWW\output -t 10000 -D C:\DynamoRIO8.0.18460\bin32 -- -coverage_module common_lib.dll -coverage_module Jw_win.exe -target_module Jw_win.exe -target_offset 0x0283448 -fuzz_iterations 5000 -nargs 2 -call_convention thiscall -- C:\winafl_for_jwc\build32\bin\Release\JWW\Jw_win.exe @@
```

これを、管理者権限で立ち上げたたコマンドプロンプトで実行すれば良い。実行の際は、コマンドプロンプトのカレントディレクトリを```afl-fuzz.exe```のある、```C:\winafl\build32\bin\Release```に移動することも忘れてはいけない。

### 実行時の画面
```afl-fuzz.exe```を無事に実行すると、以下の様な画面が表示される。

画像aflfuzz1

各項目について順に説明する。

|process timing| |
|:---|:---|
|run time|実行開始からの経過時間|
|last new path||
|last uniq crash|新種のcrashファイル発見からの経過時間|
|last uniq hang|新種のhangファイル発見からの経過時間|

|cycle progress| |
|:---|:---|
|now processing|現進捗状況|
|paths timed out||

|stage progress||
|:---|:---|
|now truing|現選択戦略|
|stage execs|現戦略実行回数|
|total execs|総実行回数|
|execs speed|実行速度|


### 並列Fuzzing

## 結果の解析