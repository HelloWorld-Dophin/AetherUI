// 把 assets/icons/*.svg 转换成 C++ 数据表。
//
// 用法：node cpp/tools/gen_icons.js
// 输出：cpp/src/ui/icon_data.cpp（自动生成，勿手改）
//
// 数据格式：每个图标一条扁平的命令流（viewBox 0..24）：
//   0 x y               移动
//   1 x y               直线
//   2 x1 y1 x2 y2 x y   三次贝塞尔
//   3                   闭合
//
// 图标清单与显示顺序由 ICON_NAMES 决定，文件名为 name 的小写连字符形式，
// 加粗变体为同名加 `-bold` 后缀。

const fs = require('fs');
const path = require('path');

const root = path.resolve(__dirname, '..', '..');
const iconDir = path.join(root, 'assets', 'icons');

const ICON_NAMES = [
  'HOME', 'CURSOR', 'CONTROL_BUTTON', 'CHEVRON_DOWN', 'TOGGLE', 'INFO', 'WEATHER_SUNNY',
  'WEATHER_MOON', 'DISMISS', 'TEXT_FONT', 'GRID', 'COPY', 'ADD', 'SUBTRACT', 'CHECKMARK',
  'ARROW_LEFT', 'ARROW_RIGHT', 'ARROW_UP', 'ARROW_DOWN', 'ARROW_TRENDING', 'CHEVRON_LEFT',
  'CHEVRON_RIGHT', 'CHEVRON_UP', 'SEARCH', 'SETTINGS', 'PERSON', 'PEOPLE', 'MAIL', 'CALENDAR_LTR',
  'CLOCK', 'CAMERA', 'IMAGE', 'DOCUMENT', 'FOLDER', 'FOLDER_OPEN', 'SAVE', 'DELETE', 'EDIT',
  'SHARE', 'LINK', 'ATTACH', 'SEND', 'ARROW_DOWNLOAD', 'ARROW_UPLOAD', 'CLOUD', 'STAR', 'HEART',
  'BOOKMARK', 'FILTER', 'ARROW_SORT', 'MORE_HORIZONTAL', 'MORE_VERTICAL', 'NAVIGATION', 'LIST',
  'TABLE', 'PIN', 'ALERT', 'QUESTION_CIRCLE', 'WARNING', 'ERROR_CIRCLE', 'CHECKMARK_CIRCLE',
  'DISMISS_CIRCLE', 'ADD_CIRCLE', 'PLAY', 'PAUSE', 'STOP', 'SPEAKER_2', 'VIDEO', 'CALL', 'CHAT',
  'COMMENT', 'THUMB_LIKE', 'THUMB_DISLIKE', 'GLOBE', 'LOCATION', 'MAP', 'ROCKET', 'LIGHTBULB',
  'PAINT_BRUSH', 'PEN', 'ERASER', 'CUT', 'CLIPBOARD', 'CODE', 'BUG', 'WRENCH', 'KEYBOARD', 'LAPTOP',
  'PHONE', 'TABLET', 'PLUG_CONNECTED', 'BATTERY_10', 'WIFI_1', 'BLUETOOTH', 'LOCK_CLOSED',
  'LOCK_OPEN', 'KEY', 'SHIELD', 'EYE', 'EYE_OFF', 'WEATHER_CLOUDY', 'UMBRELLA', 'SPARKLE', 'GIFT',
  'CART', 'WALLET', 'BUILDING', 'BRIEFCASE', 'BACKPACK', 'HAT_GRADUATION', 'FOOD', 'DRINK_COFFEE',
  'LEAF_ONE', 'BOOK_OPEN', 'NOTEBOOK', 'READING_LIST', 'SQUARE', 'SQUARE_MULTIPLE',
  'TEXT_BULLET_LIST', 'TEXT_BOLD', 'TEXT_ITALIC', 'TEXT_UNDERLINE', 'TEXT_ALIGN_LEFT',
  'TEXT_ALIGN_CENTER', 'LOCAL_LANGUAGE', 'TRANSLATE',
];

function file_of(name) {
  return name.toLowerCase().replace(/_/g, '-');
}

const entries = ICON_NAMES.map((name) => {
  const file = file_of(name);
  if (!fs.existsSync(path.join(iconDir, file + '.svg'))) {
    throw new Error('缺少图标文件：' + file + '.svg');
  }
  if (!fs.existsSync(path.join(iconDir, file + '-bold.svg'))) {
    throw new Error('缺少加粗图标文件：' + file + '-bold.svg');
  }
  return { name, regular: file, bold: file + '-bold' };
});

// ---------------------------------------------------------------- SVG 路径解析
function tokenize(d) {
  const out = [];
  const re = /([MmLlHhVvCcSsQqTtAaZz])|(-?\d*\.?\d+(?:[eE][-+]?\d+)?)/g;
  let t;
  while ((t = re.exec(d)) !== null) {
    out.push(t[1] ? t[1] : parseFloat(t[2]));
  }
  return out;
}

function arcToCubics(x0, y0, rx, ry, rot, large, sweep, x1, y1) {
  rx = Math.abs(rx);
  ry = Math.abs(ry);
  if (rx === 0 || ry === 0) {
    return [[x0, y0, x1, y1, x1, y1]];
  }
  const phi = (rot * Math.PI) / 180;
  const cosP = Math.cos(phi);
  const sinP = Math.sin(phi);
  const dx = (x0 - x1) / 2;
  const dy = (y0 - y1) / 2;
  const x1p = cosP * dx + sinP * dy;
  const y1p = -sinP * dx + cosP * dy;
  let rx2 = rx * rx;
  let ry2 = ry * ry;
  const lambda = (x1p * x1p) / rx2 + (y1p * y1p) / ry2;
  if (lambda > 1) {
    const s = Math.sqrt(lambda);
    rx *= s;
    ry *= s;
    rx2 = rx * rx;
    ry2 = ry * ry;
  }
  let sign = large === sweep ? -1 : 1;
  const num = rx2 * ry2 - rx2 * y1p * y1p - ry2 * x1p * x1p;
  const den = rx2 * y1p * y1p + ry2 * x1p * x1p;
  const co = sign * Math.sqrt(Math.max(0, num / den));
  const cxp = (co * rx * y1p) / ry;
  const cyp = (-co * ry * x1p) / rx;
  const cx = cosP * cxp - sinP * cyp + (x0 + x1) / 2;
  const cy = sinP * cxp + cosP * cyp + (y0 + y1) / 2;
  const angle = (ux, uy, vx, vy) => {
    const dot = ux * vx + uy * vy;
    const len = Math.sqrt(ux * ux + uy * uy) * Math.sqrt(vx * vx + vy * vy);
    let a = Math.acos(Math.min(1, Math.max(-1, dot / len)));
    if (ux * vy - uy * vx < 0) a = -a;
    return a;
  };
  const theta1 = angle(1, 0, (x1p - cxp) / rx, (y1p - cyp) / ry);
  let dtheta = angle((x1p - cxp) / rx, (y1p - cyp) / ry, (-x1p - cxp) / rx, (-y1p - cyp) / ry);
  if (!sweep && dtheta > 0) dtheta -= 2 * Math.PI;
  if (sweep && dtheta < 0) dtheta += 2 * Math.PI;

  const segments = Math.max(1, Math.ceil(Math.abs(dtheta) / (Math.PI / 2)));
  const delta = dtheta / segments;
  const t = (4 / 3) * Math.tan(delta / 4);
  const out = [];
  let th = theta1;
  let px = x0;
  let py = y0;
  for (let i = 0; i < segments; i++) {
    const th2 = th + delta;
    const cosT1 = Math.cos(th);
    const sinT1 = Math.sin(th);
    const cosT2 = Math.cos(th2);
    const sinT2 = Math.sin(th2);
    const e1x = cx + cosP * rx * cosT1 - sinP * ry * sinT1;
    const e1y = cy + sinP * rx * cosT1 + cosP * ry * sinT1;
    const e2x = cx + cosP * rx * cosT2 - sinP * ry * sinT2;
    const e2y = cy + sinP * rx * cosT2 + cosP * ry * sinT2;
    const d1x = -cosP * rx * sinT1 - sinP * ry * cosT1;
    const d1y = -sinP * rx * sinT1 + cosP * ry * cosT1;
    const d2x = -cosP * rx * sinT2 - sinP * ry * cosT2;
    const d2y = -sinP * rx * sinT2 + cosP * ry * cosT2;
    out.push([px + t * d1x, py + t * d1y, e2x - t * d2x, e2y - t * d2y, e2x, e2y]);
    px = e2x;
    py = e2y;
    th = th2;
  }
  return out;
}

function parsePath(d) {
  const t = tokenize(d);
  const cmds = [];
  let i = 0;
  let x = 0;
  let y = 0;
  let sx = 0;
  let sy = 0;
  let lastCtrlX = 0;
  let lastCtrlY = 0;
  let lastCmd = '';
  const num = () => t[i++];
  while (i < t.length) {
    let cmd = typeof t[i] === 'string' ? t[i++] : null;
    if (cmd === null) {
      // 隐式重复上一个命令（M 之后视作 L）。
      cmd = lastCmd === 'M' ? 'L' : lastCmd === 'm' ? 'l' : lastCmd;
    }
    const rel = cmd === cmd.toLowerCase();
    switch (cmd.toUpperCase()) {
      case 'M': {
        x = rel ? x + num() : num();
        y = rel ? y + num() : num();
        sx = x;
        sy = y;
        cmds.push([0, x, y]);
        break;
      }
      case 'L': {
        x = rel ? x + num() : num();
        y = rel ? y + num() : num();
        cmds.push([1, x, y]);
        break;
      }
      case 'H': {
        x = rel ? x + num() : num();
        cmds.push([1, x, y]);
        break;
      }
      case 'V': {
        y = rel ? y + num() : num();
        cmds.push([1, x, y]);
        break;
      }
      case 'C': {
        const x1 = rel ? x + num() : num();
        const y1 = rel ? y + num() : num();
        const x2 = rel ? x + num() : num();
        const y2 = rel ? y + num() : num();
        const x3 = rel ? x + num() : num();
        const y3 = rel ? y + num() : num();
        cmds.push([2, x1, y1, x2, y2, x3, y3]);
        lastCtrlX = x2;
        lastCtrlY = y2;
        x = x3;
        y = y3;
        break;
      }
      case 'S': {
        const x1 = 2 * x - lastCtrlX;
        const y1 = 2 * y - lastCtrlY;
        const x2 = rel ? x + num() : num();
        const y2 = rel ? y + num() : num();
        const x3 = rel ? x + num() : num();
        const y3 = rel ? y + num() : num();
        cmds.push([2, x1, y1, x2, y2, x3, y3]);
        lastCtrlX = x2;
        lastCtrlY = y2;
        x = x3;
        y = y3;
        break;
      }
      case 'Q': {
        const qx = rel ? x + num() : num();
        const qy = rel ? y + num() : num();
        const x3 = rel ? x + num() : num();
        const y3 = rel ? y + num() : num();
        cmds.push([2, x + (2 / 3) * (qx - x), y + (2 / 3) * (qy - y),
          x3 + (2 / 3) * (qx - x3), y3 + (2 / 3) * (qy - y3), x3, y3]);
        lastCtrlX = qx;
        lastCtrlY = qy;
        x = x3;
        y = y3;
        break;
      }
      case 'T': {
        const qx = 2 * x - lastCtrlX;
        const qy = 2 * y - lastCtrlY;
        const x3 = rel ? x + num() : num();
        const y3 = rel ? y + num() : num();
        cmds.push([2, x + (2 / 3) * (qx - x), y + (2 / 3) * (qy - y),
          x3 + (2 / 3) * (qx - x3), y3 + (2 / 3) * (qy - y3), x3, y3]);
        lastCtrlX = qx;
        lastCtrlY = qy;
        x = x3;
        y = y3;
        break;
      }
      case 'A': {
        const rx = num();
        const ry = num();
        const rot = num();
        const large = num();
        const sweep = num() ? 1 : 0;
        const x1 = rel ? x + num() : num();
        const y1 = rel ? y + num() : num();
        for (const c of arcToCubics(x, y, rx, ry, rot, large, sweep, x1, y1)) {
          cmds.push([2, c[0], c[1], c[2], c[3], c[4], c[5]]);
        }
        x = x1;
        y = y1;
        break;
      }
      case 'Z': {
        cmds.push([3]);
        x = sx;
        y = sy;
        break;
      }
      default:
        throw new Error('未知命令 ' + cmd);
    }
    lastCmd = cmd;
  }
  return cmds;
}

function readSvg(file) {
  const text = fs.readFileSync(path.join(iconDir, file + '.svg'), 'utf8');
  const dMatch = /<path[^>]*\sd="([^"]+)"/.exec(text);
  const strokeMatch = /stroke-width="([\d.]+)"/.exec(text);
  return {
    cmds: parsePath(dMatch[1]),
    stroke: strokeMatch ? parseFloat(strokeMatch[1]) : 0,
  };
}

// ---------------------------------------------------------------- 生成 C++
const cache = new Map();  // file -> {cmds, stroke}
const pathList = [];
const pathIndex = new Map();

function intern(file) {
  if (pathIndex.has(file)) {
    return pathIndex.get(file);
  }
  if (!cache.has(file)) {
    cache.set(file, readSvg(file));
  }
  const id = pathList.length;
  pathList.push(cache.get(file));
  pathIndex.set(file, id);
  return id;
}

const rows = entries.map((e) => ({
  name: e.name,
  regular: intern(e.regular),
  bold: intern(e.bold),
}));

function fmt(v) {
  const s = v.toFixed(2);
  return s.replace(/\.00$/, '.0').replace(/^(-?)0\./, '$1.');
}

const lines = [];
lines.push('// 由 cpp/tools/gen_icons.js 自动生成，请勿手改。');
lines.push('// 数据来源：assets/icons/*.svg（Fluent UI System Icons, MIT）。');
lines.push('');
lines.push('#include "ui/icons.h"');
lines.push('');
lines.push('namespace aether {');
lines.push('namespace icons {');
lines.push('');

pathList.forEach((p, i) => {
  const flat = [];
  for (const c of p.cmds) {
    flat.push(...c);
  }
  lines.push('static const float kPath' + i + '[] = {');
  let chunk = '    ';
  for (const v of flat) {
    const piece = fmt(v) + 'f, ';
    if (chunk.length + piece.length > 100) {
      lines.push(chunk.trimEnd());
      chunk = '    ';
    }
    chunk += piece;
  }
  if (chunk.trim().length > 0) {
    lines.push(chunk.trimEnd());
  }
  lines.push('};');
  lines.push('');
});

lines.push('const IconPath kIconPaths[] = {');
pathList.forEach((p, i) => {
  let count = 0;
  for (const c of p.cmds) {
    count += 1 + c.length - 1;
  }
  lines.push('    {kPath' + i + ', ' + count + ', ' + fmt(p.stroke) + 'f},');
});
lines.push('};');
lines.push('');
lines.push('const int kIconPathCount = ' + pathList.length + ';');
lines.push('');
lines.push('const IconEntry kIconEntries[] = {');
for (const r of rows) {
  lines.push('    {"icons::' + r.name + '", ' + r.regular + ', ' + r.bold + '},');
}
lines.push('};');
lines.push('');
lines.push('const int kIconEntryCount = ' + rows.length + ';');
lines.push('');
lines.push('}  // namespace icons');
lines.push('}  // namespace aether');
lines.push('');

fs.writeFileSync(path.join(root, 'cpp', 'src', 'ui', 'icon_data.cpp'), '\ufeff' + lines.join('\n'), 'utf8');
console.log('图标数据：' + pathList.length + ' 条路径，' + rows.length + ' 个条目');
