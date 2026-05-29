# Logic

## 目的

この文書は、前景にブレンドする背景由来の代表色を、scene-space の幾何情報に基づいて求めるためのロジックを定義します。

目的は、背景 Source を単に重ねるのではなく、背景オブジェクトからの光が前景オブジェクトの輪郭に回り込むような 2D スクリーン空間近似を与えることです。

現行実装は前景ピクセル近傍の scene-space 背景サンプルを使いますが、本方針では次を分離して扱います。

- 幾何が決めるもの: どこを参照するか
- 背景テクスチャが決めるもの: 実際の色そのもの

## モデル化の前提

- OBS scene は 2D 平面として扱う
- 前景オブジェクトの形状は前景 Source のアルファチャンネルで定義する
- 想定ユースケースは、背景用の動画・静止画・ゲーム画面などを 1 つの Source として配置する構成である
- 背景オブジェクトは、scene 上の矩形領域を持つ面光源近似として扱う
- 背景矩形は raw の Source サイズではなく、scene item の実際の box を使う
- 運用制約として、背景 Source と背景 scene item は 1:1 であることを前提にする
- 1 つの背景 Source から複数の scene item を作成して再利用する構成は動作保証外とする
- 背景色は背景テクスチャから取得し、幾何はサンプル座標と重みだけを決める
- scene item の解決に失敗した場合は、現行の scene-space 背景サンプルへ fallback する

この近似は 3D の物理ベース照明ではなく、OBS の 2D scene に対する screen-space proxy です。

## 変数

- $C = (W, H)$: OBS canvas サイズ（pixels）
- $u = (u_x, u_y)$: 前景ピクセルの正規化 UV 座標
- $S_f = (W_f, H_f)$: 前景 Source のサイズ（pixels）
- $A_f(u)$: 前景 Source のアルファ値、$A_f \in [0, 1]$
- $\tau$: アルファ境界判定の閾値
- $o_f, a_f, b_f$: 前景 UV を scene-space UV へ写す基底
- $B(x)$: 背景テクスチャを scene-space 上で参照した色
- $R_b$: 背景オブジェクトの矩形近似
- $c_b$: 背景矩形の中心座標（scene pixels）
- $e_x, e_y$: 背景矩形の単位基底ベクトル
- $h_x, h_y$: 背景矩形の半サイズ（scene pixels）
- $r_e$: 前景境界探索半径（foreground texels）
- $\lambda$: 前景輪郭から光源側へ押し出すオフセット量（scene pixels）
- $r_s$: 背景代表色のサンプリング半径（scene pixels）

## 座標系

### 前景 UV から scene-space への写像

現行実装の `scene_uv_origin`, `scene_uv_x`, `scene_uv_y` は、正規化された scene-space UV 基底です。これを canvas ピクセル座標へ戻すと、前景ピクセルの scene 位置 $p_f(u)$ は次で表せます。

$$
p_f(u) =
\begin{bmatrix}
W \\
H
\end{bmatrix}
\odot
\left(o_f + a_f u_x + b_f u_y\right)
$$

ここで $\odot$ は要素ごとの積です。

前景 UV の微小変化を scene pixels へ写すヤコビアンは、次の $2 \times 2$ 行列で表せます。

$$
J_f =
\begin{bmatrix}
W a_{f,x} & W b_{f,x} \\
H a_{f,y} & H b_{f,y}
\end{bmatrix}
$$

### 背景矩形の表現

背景オブジェクトを scene 上の回転矩形として表すと、背景矩形 $R_b$ は次です。

$$
R_b = \left\{ c_b + e_x s_x + e_y s_y \mid s_x \in [-h_x, h_x],\ s_y \in [-h_y, h_y] \right\}
$$

ここで $2 h_x, 2 h_y$ は背景矩形の実寸です。raw の `base_width` / `base_height` ではなく、scene item の bounds / crop / scale を反映した box を使うのが前提です。

## 前景輪郭の幾何

### ローカル法線の推定

前景アルファからローカル法線を推定します。前景 1 texel を

$$
t_f = \left(\frac{1}{W_f}, \frac{1}{H_f}\right)
$$

とすると、アルファ勾配は近似的に次です。

$$
g(u) =
\begin{bmatrix}
A_f\left(u + (t_{f,x}, 0)\right) - A_f\left(u - (t_{f,x}, 0)\right) \\
A_f\left(u + (0, t_{f,y})\right) - A_f\left(u - (0, t_{f,y})\right)
\end{bmatrix}
$$

アルファは物体内部で高く、外側で低い前提なので、外向きローカル法線 $n_l(u)$ は次で定義します。

$$
n_l(u) = -\frac{g(u)}{\max(\|g(u)\|, \varepsilon)}
$$

### 輪郭位置の推定

現在のピクセル $u$ が前景内部にあるとして、外向き法線方向に境界を探索します。探索 texel 数 $k_e(u)$ は次です。

$$
k_e(u) = \min \left\{ k \in [0, r_e] \mid A_f\left(u + k (t_f \odot n_l(u))\right) < \tau \right\}
$$

探索で境界が見つからない場合は $k_e(u) = r_e$ とします。

輪郭 UV は

$$
u_e = u + k_e(u) \left(t_f \odot n_l(u)\right)
$$

輪郭の scene-space 座標は

$$
p_e = p_f(u_e)
$$

です。

また、scene-space の外向き法線は、ローカル法線をヤコビアンで写して正規化します。

$$
n_s = \frac{J_f \left(t_f \odot n_l(u)\right)}{\max\left(\left\|J_f \left(t_f \odot n_l(u)\right)\right\|, \varepsilon\right)}
$$

輪郭に近いほど効果を強くするため、輪郭重み $w_e$ は次で与えます。

$$
w_e = \operatorname{saturate}\left(1 - \frac{k_e(u)}{r_e}\right)
$$

## 背景矩形への投影

光源の代表参照位置は、輪郭点そのものではなく、輪郭から外向きに少し押し出した点から、さらに外向き法線に沿って背景矩形の境界へ到達する位置として求めます。

まず scene-space の seed 点 $s$ を

$$
s = p_e + \lambda n_s
$$

とします。

次に、$s$ を背景矩形のローカル座標へ落とした値を

$$
\alpha_x = \langle d, e_x \rangle, \qquad \alpha_y = \langle d, e_y \rangle
$$

外向き法線の背景矩形ローカル成分を

$$
\beta_x = \langle n_s, e_x \rangle, \qquad \beta_y = \langle n_s, e_y \rangle
$$

とします。ここで

$$
d = s - c_b
$$

seed $s$ が背景矩形の内側、すなわち $|\alpha_x| < h_x$ かつ $|\alpha_y| < h_y$ のときは、$s + t n_s$ が最初に背景矩形の境界へ到達する正の $t$ を使います。

$$
t_{\mathrm{exit}} = \min \left(\left\{ \frac{\operatorname{sign}(\beta_x) h_x - \alpha_x}{\beta_x} \mid \beta_x \neq 0,\ t > 0 \right\} \cup \left\{ \frac{\operatorname{sign}(\beta_y) h_y - \alpha_y}{\beta_y} \mid \beta_y \neq 0,\ t > 0 \right\}\right)
$$

$$
q = s + t_{\mathrm{exit}} n_s
$$

一方、seed $s$ が背景矩形の外側にある場合は、従来どおり最近傍点を使います。

$$
q = c_b
+ e_x \operatorname{clamp}(\alpha_x, -h_x, h_x)
+ e_y \operatorname{clamp}(\alpha_y, -h_y, h_y)
$$

ここで $\langle \cdot, \cdot \rangle$ は内積です。

この定義により、背景オブジェクトが前景と重なっている場合でも、背景矩形の内側にある局所点そのものを返さず、外向き法線方向に見た背景境界側の代表位置を得られます。単に背景サイズだけを使うのではなく、背景矩形の中心 $c_b$ と基底 $e_x, e_y$ も必要です。

## 背景代表色の算出

輪郭点から背景へのベクトルを

$$
\ell = q - p_e
$$

距離を

$$
d_b = \|\ell\|
$$

方向を

$$
\hat{\ell} = \frac{\ell}{\max(d_b, \varepsilon)}
$$

とします。

背景矩形の大きさに応じてサンプリング半径を決めます。初期値としては矩形面積の平方根に比例させるのが扱いやすいです。

$$
r_s = \operatorname{clamp}\left(k_s \sqrt{(2 h_x)(2 h_y)}, r_{\min}, r_{\max}\right)
$$

極端に細長い背景では過大評価になりやすいため、その場合は $\min(2 h_x, 2 h_y)$ ベースへ切り替えてもよいです。

背景の代表色 $L_b(q)$ は、背景テクスチャを $q$ 周辺で平均して求めます。

$$
L_b(q) = \frac{\sum_i w_i\, B\left(q + \Delta_i(r_s)\right)}{\sum_i w_i}
$$

ここで $\Delta_i(r_s)$ はサンプリングオフセットです。5 点や 9 点の固定タップでもよく、必要に応じてガウシアン近似にしてもかまいません。

ただし current implementation では、OBS の effect compiler が projected path 内の複雑な multi-tap sampling で不安定化したため、shader 上は $q$ の単一点サンプルを使っています。multi-tap 化は将来拡張として残し、まずは projected 参照位置の妥当性を優先しています。

また current implementation では、背景色の参照とブレンドは輪郭探索で $w_e > 0$ になったピクセルに限定します。前景中央のように輪郭から十分離れたピクセルは、projected path と fallback path のどちらにも入れず、元の前景色を維持します。

projected path が使えない場合の fallback 参照も、現在ピクセル $u$ ではなく輪郭位置 $u_e$ を起点に行います。これにより、前景全域が背景を透かして見えるような挙動を避けます。

current implementation の fallback は sampling method ごとに役割を分けています。`Center Points` は前景全体に対する代表色として固定 5 点を使い、`Average` は輪郭位置 $u_e$ から edge width 相当の guard offset を空けた上で、外向き法線方向へ段階的に進んだ複数点を 1 次元に平均します。さらに遠い点ほど重みを強めることで、`Average` でも「輪郭の外側」を主に参照し、前景と重なっている背景領域や輪郭接線方向の内側成分を平均中心にしてしまう挙動を避けます。

背景テクスチャを canvas サイズでレンダリングしている場合、texture UV は

$$
\left(\frac{q_x}{W}, \frac{q_y}{H}\right)
$$

で求めます。

## ブレンド重み

物理現象の近似として、少なくとも次の 3 要素を重みへ入れます。

- 輪郭に近いか
- 輪郭法線が背景方向を向いているか
- 背景矩形までの距離が近いか

### 角度項

$$
w_\theta = \operatorname{saturate}(\langle n_s, \hat{\ell} \rangle)
$$

### 距離項

$$
w_d = \exp\left(-\frac{d_b}{\sigma_d}\right)
$$

### 最終重み

$$
w = \text{intensity} \cdot w_e \cdot w_\theta^{\gamma} \cdot w_d
$$

ここで $\gamma$ は角度依存を強める指数です。

## 最終色

背景由来の環境色を

$$
c_{env} = \text{tint} \odot L_b(q)
$$

とします。

既存の blend mode をそのまま使うなら、前景色 $c_{fg}$ からの最終色は

$$
c_{blend} = \operatorname{Blend}(c_{fg}, c_{env}, \text{mode})
$$

$$
c_{out} = \operatorname{lerp}(c_{fg}, c_{blend}, w)
$$

です。

現行の light wrap / rim light を分けて維持する場合でも、参照色 $c_{env}$ は同じ式でよく、wrap と rim の係数だけ別に掛ければ済みます。

## 実装への対応

### CPU 側で必要な追加情報

- 前景 Source の scene-space UV 基底
  - これは現行の `SceneUvMapping` でほぼ足りている
- 背景 Source の scene item 解決
  - 前景と同様に program scene / preview scene / scene 一覧から探索する
- 背景矩形の scene-space 表現
  - `obs_sceneitem_get_box_transform` を優先候補にする
  - 必要なら `obs_sceneitem_get_info2` と crop / bounds から補完する
- shader へ渡す uniform
  - 背景矩形中心
  - 背景矩形の X / Y 基底
  - 背景矩形 half extent
  - 背景矩形の解決可否

### Shader 側で行う処理

1. 前景アルファからローカル法線 $n_l$ を求める
2. 輪郭探索で $u_e$ と $w_e$ を求める
3. 前景 scene-space 基底で $p_e$ と $n_s$ を求める
4. 背景矩形へ投影して $q$ を求める
5. $q$ 周辺の背景色 $L_b(q)$ を取る
6. $w_\theta$, $w_d$ を計算して最終重み $w$ を求める
7. 既存 blend mode へ渡す

## この方式が必要とする変数

ユーザー要求の 3 変数は妥当ですが、そのままでは不足があります。

- OBS スクリーンサイズ
  - 必須
  - 正規化座標と pixel 距離を相互変換するために使う
- 背景オブジェクトのサイズ
  - 必須
  - ただしサイズだけでは足りず、位置と回転も必要
- 前景オブジェクトのアルファ境界座標
  - 必須
  - ただし境界法線も必要

したがって、実際に必要な最小変数集合は次です。

- canvas サイズ $C$
- 前景輪郭点 $p_e$
- 前景輪郭法線 $n_s$
- 背景矩形の中心 $c_b$
- 背景矩形の基底 $e_x, e_y$
- 背景矩形の半サイズ $h_x, h_y$

## 制約と fallback

- 背景 Source は scene 内で 1 個の scene item にだけ対応していることを前提とする
- 同じ背景 Source を複数の scene item として配置した場合、どの矩形を光源として使うべきか一意に決められないため、動作保証外とする
- 非矩形の背景 Source は、本方式ではまず矩形面光源に潰して近似する
- 背景 item の crop / bounds / nested scene を無視すると矩形がずれる
- 背景矩形が解決できない場合は、現行の scene-space 背景サンプルへ戻す
- 本方式は 2D screen-space proxy なので、被写体の奥行きや自己遮蔽は扱わない

## 実装上の推奨順序

1. 前景の輪郭点 $p_e$ と法線 $n_s$ を安定して出す
2. 背景 scene item を解決して矩形 $R_b$ を得る
3. 最近傍投影 $q = \Pi_{R_b}(p_e + \lambda n_s)$ を実装する
4. $q$ 周辺の代表色 $L_b(q)$ を背景サイズ連動の半径で取得する
5. 距離項と角度項を入れて最終ブレンド重み $w$ を組む

この順序で進めれば、まず「どの背景位置を見るべきか」を安定させ、その上で「どの程度回り込ませるか」を調整できます。
