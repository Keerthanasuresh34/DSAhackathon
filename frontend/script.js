/**
 * The Museum's Vanishing Family Trees — Frontend Script
 * Member 3 Implementation & Member 4 Full Integration
 */

// API Configuration
const API_BASE = window.location.origin.includes('localhost') || window.location.origin.includes('127.0.0.1')
  ? ''
  : ''; // relative path for same-origin Flask backend

// State Store
let currentTreeData = null;
let originalTokensCache = "A B # D # # C # #";
let isAnimatingTraversal = false;

// DOM Elements
const treeTokensInput = document.getElementById('treeTokensInput');
const treePresetSelect = document.getElementById('treePresetSelect');
const loadPresetBtn = document.getElementById('loadPresetBtn');
const clearTokensBtn = document.getElementById('clearTokensBtn');
const tokenCounter = document.getElementById('tokenCounter');

const createTreeBtn = document.getElementById('createTreeBtn');
const serializeBtn = document.getElementById('serializeBtn');
const exportArchiveBtn = document.getElementById('exportArchiveBtn');

const serializationCard = document.getElementById('serializationCard');
const serializedOutput = document.getElementById('serializedOutput');
const copySerializedBtn = document.getElementById('copySerializedBtn');

const nodeCountMetric = document.getElementById('nodeCountMetric');
const heightMetric = document.getElementById('heightMetric');
const sentinelCountMetric = document.getElementById('sentinelCountMetric');
const parityMetric = document.getElementById('parityMetric');

const preorderDisplay = document.getElementById('preorderDisplay');
const inorderDisplay = document.getElementById('inorderDisplay');
const postorderDisplay = document.getElementById('postorderDisplay');

const toggleAsciiBtn = document.getElementById('toggleAsciiBtn');
const asciiDisplay = document.getElementById('asciiDisplay');

const treeSvg = document.getElementById('treeSvg');
const treeEmptyState = document.getElementById('treeEmptyState');
const showNullNodesToggle = document.getElementById('showNullNodesToggle');
const resetZoomBtn = document.getElementById('resetZoomBtn');

const archiveTextInput = document.getElementById('archiveTextInput');
const validateArchiveBtn = document.getElementById('validateArchiveBtn');
const restoreArchiveBtn = document.getElementById('restoreArchiveBtn');
const diagnosticCard = document.getElementById('diagnosticCard');
const diagnosticIcon = document.getElementById('diagnosticIcon');
const diagnosticTitle = document.getElementById('diagnosticTitle');
const diagnosticMessage = document.getElementById('diagnosticMessage');
const diagnosticStats = document.getElementById('diagnosticStats');

const compareTreesBtn = document.getElementById('compareTreesBtn');
const runRoundtripDemoBtn = document.getElementById('runRoundtripDemoBtn');
const comparisonCard = document.getElementById('comparisonCard');
const compIcon = document.getElementById('compIcon');
const compResultText = document.getElementById('compResultText');
const compOrigNodes = document.getElementById('compOrigNodes');
const compRestNodes = document.getElementById('compRestNodes');
const compStructStatus = document.getElementById('compStructStatus');
const compOrigPre = document.getElementById('compOrigPre');
const compRestPre = document.getElementById('compRestPre');
const compPreStatus = document.getElementById('compPreStatus');
const compOrigIn = document.getElementById('compOrigIn');
const compRestIn = document.getElementById('compRestIn');
const compInStatus = document.getElementById('compInStatus');
const compOrigPost = document.getElementById('compOrigPost');
const compRestPost = document.getElementById('compRestPost');
const compPostStatus = document.getElementById('compPostStatus');

const toastNotification = document.getElementById('toastNotification');
const themeToggleBtn = document.getElementById('themeToggleBtn');
const cEngineStatus = document.getElementById('cEngineStatus');
const flaskStatus = document.getElementById('flaskStatus');

// -------------------------------------------------------------
// Toast Notifications
// -------------------------------------------------------------
function showToast(message, duration = 3500) {
  toastNotification.textContent = message;
  toastNotification.classList.remove('hidden');
  toastNotification.style.opacity = '1';
  toastNotification.style.transform = 'translateY(0)';

  setTimeout(() => {
    toastNotification.style.opacity = '0';
    toastNotification.style.transform = 'translateY(10px)';
    setTimeout(() => toastNotification.classList.add('hidden'), 300);
  }, duration);
}

// -------------------------------------------------------------
// Health Check on Initialization
// -------------------------------------------------------------
async function checkBackendHealth() {
  try {
    const res = await fetch(`${API_BASE}/api/health`);
    const data = await res.json();
    if (res.ok && data.status === 'healthy') {
      cEngineStatus.innerHTML = `<span class="dot"></span> C Engine: Online (${data.c_engine.engine || 'C11'})`;
      flaskStatus.innerHTML = `<span class="dot"></span> Flask API: Connected`;
    } else {
      cEngineStatus.innerHTML = `<span class="dot" style="background:#ff7b72"></span> C Engine: Degraded`;
    }
  } catch (err) {
    cEngineStatus.innerHTML = `<span class="dot" style="background:#ff7b72"></span> Backend: Offline`;
  }
}

// -------------------------------------------------------------
// Token Counter and Validator Preview
// -------------------------------------------------------------
function updateTokenCounter() {
  const text = treeTokensInput.value.trim();
  if (!text) {
    tokenCounter.textContent = 'Tokens: 0 | Nodes: 0 | Sentinels: 0';
    return;
  }
  const tokens = text.split(/\s+/).filter(t => t.length > 0);
  const nulls = tokens.filter(t => t === '#').length;
  const nodes = tokens.length - nulls;
  const expectedNulls = nodes + 1;
  const isParityOk = tokens.length > 0 && nulls === expectedNulls;

  tokenCounter.textContent = `Tokens: ${tokens.length} | Monarch Nodes: ${nodes} | Null Sentinels: ${nulls} (${isParityOk ? 'Parity Valid' : 'Parity Open/Pending'})`;
}

treeTokensInput.addEventListener('input', updateTokenCounter);

clearTokensBtn.addEventListener('click', () => {
  treeTokensInput.value = '';
  updateTokenCounter();
});

loadPresetBtn.addEventListener('click', () => {
  treeTokensInput.value = treePresetSelect.value;
  updateTokenCounter();
  showToast(`Loaded preset: ${treePresetSelect.options[treePresetSelect.selectedIndex].text.split('(')[0]}`);
  createTreeBtn.click();
});

// -------------------------------------------------------------
// Theme Toggle
// -------------------------------------------------------------
themeToggleBtn.addEventListener('click', () => {
  const isParchment = document.body.classList.toggle('theme-parchment');
  themeToggleBtn.textContent = isParchment ? '🏛️ Obsidian Hall Mode' : '📜 Parchment Mode';
  showToast(`Switched to ${isParchment ? 'Ancient Parchment' : 'Imperial Obsidian'} exhibition mode.`);
  if (currentTreeData) {
    renderSvgTree(currentTreeData);
  }
});

// -------------------------------------------------------------
// API: Construct & Visualize Tree
// -------------------------------------------------------------
createTreeBtn.addEventListener('click', async () => {
  const tokens = treeTokensInput.value.trim();
  if (!tokens) {
    showToast('⚠️ Please enter lineage tokens first.');
    return;
  }

  originalTokensCache = tokens;

  try {
    createTreeBtn.disabled = true;
    createTreeBtn.innerHTML = '<span class="btn-icon">⏳</span> Constructing...';

    const res = await fetch(`${API_BASE}/api/tree/create`, {
      method: 'POST',
      headers: { 'Content-Type': 'application/json' },
      body: JSON.stringify({ tokens })
    });

    const data = await res.json();

    if (!res.ok || !data.success) {
      showToast(`❌ Construction failed: ${data.error || 'Invalid tokens'}`);
      return;
    }

    // Populate Metrics
    nodeCountMetric.textContent = data.node_count;
    heightMetric.textContent = data.height;
    const tokensArr = tokens.split(/\s+/);
    const nulls = tokensArr.filter(t => t === '#').length;
    sentinelCountMetric.textContent = nulls;
    parityMetric.textContent = nulls === data.node_count + 1 ? 'Balanced (N+1)' : 'Irregular';

    // Populate Traversals
    preorderDisplay.textContent = data.preorder || '(empty)';
    inorderDisplay.textContent = data.inorder || '(empty)';
    postorderDisplay.textContent = data.postorder || '(empty)';

    // Populate ASCII Display
    asciiDisplay.textContent = data.ascii_art || '';

    // Render SVG Tree
    currentTreeData = data.tree_json;
    renderSvgTree(data.tree_json);

    // Auto-populate vault with raw tokens if empty
    if (!archiveTextInput.value.trim()) {
      archiveTextInput.value = tokens;
    }

    showToast('✨ Royal family tree constructed successfully!');
  } catch (err) {
    showToast(`❌ Network/Server Error: ${err.message}`);
  } finally {
    createTreeBtn.disabled = false;
    createTreeBtn.innerHTML = '<span class="btn-icon">⚡</span> Construct & Visualize Tree';
  }
});

// -------------------------------------------------------------
// API: Serialize Tokens
// -------------------------------------------------------------
serializeBtn.addEventListener('click', async () => {
  const tokens = treeTokensInput.value.trim();
  if (!tokens) {
    showToast('⚠️ Please enter tree tokens to serialize.');
    return;
  }

  try {
    const res = await fetch(`${API_BASE}/api/tree/serialize`, {
      method: 'POST',
      headers: { 'Content-Type': 'application/json' },
      body: JSON.stringify({ tokens })
    });
    const data = await res.json();
    if (!res.ok || !data.success) {
      showToast(`❌ Serialization failed: ${data.error || 'Unknown error'}`);
      return;
    }

    serializedOutput.textContent = data.serialized;
    serializationCard.classList.remove('hidden');
    archiveTextInput.value = data.serialized;
    showToast('📦 Tree serialized into archival stream!');
  } catch (err) {
    showToast(`❌ Error: ${err.message}`);
  }
});

copySerializedBtn.addEventListener('click', () => {
  navigator.clipboard.writeText(serializedOutput.textContent);
  showToast('📋 Serialized stream copied to clipboard!');
});

// -------------------------------------------------------------
// API: Export to Archival File
// -------------------------------------------------------------
exportArchiveBtn.addEventListener('click', async () => {
  const tokens = treeTokensInput.value.trim();
  if (!tokens) {
    showToast('⚠️ Please enter tree tokens to export.');
    return;
  }

  try {
    exportArchiveBtn.disabled = true;
    const res = await fetch(`${API_BASE}/api/archive/export`, {
      method: 'POST',
      headers: { 'Content-Type': 'application/json' },
      body: JSON.stringify({
        tokens,
        filename: 'museum_dynasty_archive.txt',
        dynasty_name: 'Alexandria Ptolemaic Dynasty'
      })
    });
    const data = await res.json();

    if (!res.ok || !data.success) {
      showToast(`❌ Export failed: ${data.error}`);
      return;
    }

    // Populate Archive Vault with formal document
    archiveTextInput.value = data.archive_text;
    showToast('💾 Archival record created! Starting download...');

    // Trigger file download
    const link = document.createElement('a');
    link.href = data.download_url;
    link.download = data.filename;
    document.body.appendChild(link);
    link.click();
    document.body.removeChild(link);
  } catch (err) {
    showToast(`❌ Error: ${err.message}`);
  } finally {
    exportArchiveBtn.disabled = false;
  }
});

// -------------------------------------------------------------
// API: Validate Archive
// -------------------------------------------------------------
validateArchiveBtn.addEventListener('click', async () => {
  const archiveText = archiveTextInput.value.trim();
  if (!archiveText) {
    showToast('⚠️ Archive vault textarea is empty.');
    return;
  }

  try {
    const res = await fetch(`${API_BASE}/api/archive/validate`, {
      method: 'POST',
      headers: { 'Content-Type': 'application/json' },
      body: JSON.stringify({ archive_text: archiveText })
    });
    const data = await res.json();

    diagnosticCard.classList.remove('hidden');
    if (data.valid) {
      diagnosticCard.className = 'diagnostic-card valid';
      diagnosticIcon.textContent = '✓';
      diagnosticTitle.textContent = 'ARCHIVAL INTEGRITY VERIFIED (VALID)';
      diagnosticMessage.textContent = 'The archival token sequence complies with all structural and parity rules.';
      diagnosticStats.textContent = `Nodes: ${data.node_count} | Null Sentinels: ${data.null_count} | Total Stream Tokens: ${data.total_tokens}`;
      showToast('✓ Archive verified! 100% structurally sound.');
    } else {
      diagnosticCard.className = 'diagnostic-card invalid';
      diagnosticIcon.textContent = '⚠';
      diagnosticTitle.textContent = `CORRUPTED ARCHIVE REJECTED [${data.error_code || 'DAMAGE'}]`;
      diagnosticMessage.textContent = data.error_message || data.error || 'Structural anomalies detected in archive.';
      diagnosticStats.textContent = `Nodes Detected: ${data.node_count || 0} | Sentinels: ${data.null_count || 0} | Total Tokens: ${data.total_tokens || 0}`;
      showToast(`⚠ Validation Alert: ${data.error_message || data.error}`);
    }
  } catch (err) {
    showToast(`❌ Validation error: ${err.message}`);
  }
});

// -------------------------------------------------------------
// API: Restore Archive
// -------------------------------------------------------------
restoreArchiveBtn.addEventListener('click', async () => {
  const archiveText = archiveTextInput.value.trim();
  if (!archiveText) {
    showToast('⚠️ Archive vault textarea is empty.');
    return;
  }

  try {
    restoreArchiveBtn.disabled = true;
    restoreArchiveBtn.innerHTML = '<span class="btn-icon">⏳</span> Restoring...';

    const res = await fetch(`${API_BASE}/api/archive/restore`, {
      method: 'POST',
      headers: { 'Content-Type': 'application/json' },
      body: JSON.stringify({ archive_text: archiveText })
    });
    const data = await res.json();

    if (!res.ok || !data.success) {
      diagnosticCard.classList.remove('hidden');
      diagnosticCard.className = 'diagnostic-card invalid';
      diagnosticIcon.textContent = '✕';
      diagnosticTitle.textContent = 'RESTORATION ABORTED';
      diagnosticMessage.textContent = data.error || 'Failed to restore tree from corrupted archive.';
      showToast(`❌ Restoration failed: ${data.error}`);
      return;
    }

    // Populate Metrics
    nodeCountMetric.textContent = data.node_count;
    heightMetric.textContent = data.height;
    preorderDisplay.textContent = data.preorder || '(empty)';
    inorderDisplay.textContent = data.inorder || '(empty)';
    postorderDisplay.textContent = data.postorder || '(empty)';
    asciiDisplay.textContent = data.ascii_art || '';

    // Render Restored SVG Tree
    currentTreeData = data.tree_json;
    renderSvgTree(data.tree_json);

    diagnosticCard.classList.remove('hidden');
    diagnosticCard.className = 'diagnostic-card valid';
    diagnosticIcon.textContent = '✨';
    diagnosticTitle.textContent = 'DYNASTIC TREE RESTORED SUCCESSFULLY';
    diagnosticMessage.textContent = `Successfully recovered ${data.node_count} monarchs with height ${data.height} from archival text alone.`;
    showToast('✨ Royal family tree restored from archive!');
  } catch (err) {
    showToast(`❌ Restoration error: ${err.message}`);
  } finally {
    restoreArchiveBtn.disabled = false;
    restoreArchiveBtn.innerHTML = '<span class="btn-icon">✨</span> Restore Tree from Archive';
  }
});

// -------------------------------------------------------------
// API: Compare Trees (Isomorphism Verification)
// -------------------------------------------------------------
compareTreesBtn.addEventListener('click', async () => {
  const original = originalTokensCache || treeTokensInput.value.trim();
  const archive = archiveTextInput.value.trim();

  if (!original) {
    showToast('⚠️ Original tree tokens are missing.');
    return;
  }
  if (!archive) {
    showToast('⚠️ Restored archive text is missing.');
    return;
  }

  try {
    compareTreesBtn.disabled = true;
    const res = await fetch(`${API_BASE}/api/tree/compare`, {
      method: 'POST',
      headers: { 'Content-Type': 'application/json' },
      body: JSON.stringify({
        original_tokens: original,
        restored_archive: archive
      })
    });
    const data = await res.json();

    if (!res.ok || !data.success) {
      showToast(`❌ Comparison check failed: ${data.error}`);
      return;
    }

    comparisonCard.classList.remove('hidden');

    const isMatch = data.overall_result === 'MATCH';
    comparisonCard.className = `comparison-card ${isMatch ? 'match' : 'mismatch'}`;
    compIcon.textContent = isMatch ? '✓' : '✕';
    compResultText.textContent = isMatch
      ? 'LINEAGE TREES ARE MATHEMATICALLY IDENTICAL (100% ISOMORPHIC)'
      : 'LINEAGE DISCREPANCY DETECTED (TREES DIFFER)';

    compOrigNodes.textContent = `${data.tree1.node_count} nodes`;
    compRestNodes.textContent = `${data.tree2.node_count} nodes`;
    compStructStatus.innerHTML = data.structural_match
      ? '<span class="badge badge-pass">MATCH</span>'
      : '<span class="badge badge-fail">DIFF</span>';

    compOrigPre.textContent = data.tree1.preorder || '-';
    compRestPre.textContent = data.tree2.preorder || '-';
    compPreStatus.innerHTML = data.preorder_match
      ? '<span class="badge badge-pass">MATCH</span>'
      : '<span class="badge badge-fail">DIFF</span>';

    compOrigIn.textContent = data.tree1.inorder || '-';
    compRestIn.textContent = data.tree2.inorder || '-';
    compInStatus.innerHTML = data.inorder_match
      ? '<span class="badge badge-pass">MATCH</span>'
      : '<span class="badge badge-fail">DIFF</span>';

    compOrigPost.textContent = data.tree1.postorder || '-';
    compRestPost.textContent = data.tree2.postorder || '-';
    compPostStatus.innerHTML = data.postorder_match
      ? '<span class="badge badge-pass">MATCH</span>'
      : '<span class="badge badge-fail">DIFF</span>';

    showToast(isMatch ? '✓ Full isomorphism verified!' : '⚠ Trees differ!');
  } catch (err) {
    showToast(`❌ Error: ${err.message}`);
  } finally {
    compareTreesBtn.disabled = false;
  }
});

// -------------------------------------------------------------
// Full 10-Step Round-Trip Proof Workflow
// -------------------------------------------------------------
runRoundtripDemoBtn.addEventListener('click', async () => {
  // Step 1: Create unseen tree with both left-only and right-only branches
  const unseenTokens = "Pharaoh1 Vizier2 # Scribe3 # # HighPriest4 Commander5 # # #";
  treeTokensInput.value = unseenTokens;
  updateTokenCounter();
  showToast('Step 1/10: Loading unseen asymmetric royal dynasty...');

  // Step 2: Construct & Visualize
  createTreeBtn.click();
  await new Promise(r => setTimeout(r, 600));

  // Step 3: Serialize
  showToast('Step 2-3/10: Generating serialized archive...');
  serializeBtn.click();
  await new Promise(r => setTimeout(r, 600));

  // Step 4: Clear Memory & Canvas to simulate parchment loss
  showToast('Step 4/10: Clearing canvas & memory to simulate vanishing tree...');
  treeSvg.innerHTML = '';
  treeEmptyState.classList.remove('hidden');
  currentTreeData = null;
  nodeCountMetric.textContent = '0';
  heightMetric.textContent = '0';
  preorderDisplay.textContent = '-';
  inorderDisplay.textContent = '-';
  postorderDisplay.textContent = '-';
  await new Promise(r => setTimeout(r, 800));

  // Step 5: Restore from archive
  showToast('Step 5/10: Restoring tree strictly from archival text alone...');
  restoreArchiveBtn.click();
  await new Promise(r => setTimeout(r, 800));

  // Step 6-10: Compare and verify isomorphism
  showToast('Step 6-10/10: Running forensic isomorphism check...');
  compareTreesBtn.click();
});

// -------------------------------------------------------------
// Damage Sandbox Injectors
// -------------------------------------------------------------
document.querySelectorAll('.damage-btn').forEach(btn => {
  btn.addEventListener('click', () => {
    const type = btn.getAttribute('data-type');
    let damagedText = '';

    switch (type) {
      case 'incomplete':
        // Missing the closing null sentinel
        damagedText = "A B # D # # C #";
        showToast('Injected: Incomplete Tokens (missing closing null sentinel)');
        break;
      case 'extra':
        // Extra tokens appended after tree is resolved
        damagedText = "A B # D # # C # # EXTRA_DYNASTY_DATA";
        showToast('Injected: Extra Tokens after complete resolution');
        break;
      case 'illegal_glyph':
        // Illegal characters in identifier
        damagedText = "A B@#$ # D # # C # #";
        showToast('Injected: Corrupted Glyph with illegal symbols');
        break;
      case 'empty':
        damagedText = "";
        showToast('Injected: Empty Archive Record');
        break;
      case 'exceed_nodes':
        // Generate > 2000 nodes
        const nodes = [];
        for (let i = 0; i < 2005; i++) {
          nodes.push(`N${i}`);
          nodes.push('#');
        }
        nodes.push('#');
        damagedText = nodes.join(' ');
        showToast('Injected: Archive with 2,005 nodes (>2000 ceiling)');
        break;
    }

    archiveTextInput.value = damagedText;
    validateArchiveBtn.click();
  });
});

// -------------------------------------------------------------
// ASCII Display Toggle
// -------------------------------------------------------------
toggleAsciiBtn.addEventListener('click', () => {
  const isHidden = asciiDisplay.classList.toggle('hidden');
  toggleAsciiBtn.textContent = isHidden ? 'Show ASCII Tree Diagram' : 'Hide ASCII Tree Diagram';
});

// -------------------------------------------------------------
// Traversal Stepping & Animation
// -------------------------------------------------------------
document.querySelectorAll('.play-traversal-btn').forEach(btn => {
  btn.addEventListener('click', async () => {
    if (isAnimatingTraversal) return;
    const type = btn.getAttribute('data-traversal');
    let seqText = '';

    if (type === 'preorder') seqText = preorderDisplay.textContent;
    if (type === 'inorder') seqText = inorderDisplay.textContent;
    if (type === 'postorder') seqText = postorderDisplay.textContent;

    if (!seqText || seqText === '-' || seqText.includes('empty')) {
      showToast('⚠️ No active tree to animate.');
      return;
    }

    const labels = seqText.trim().split(/\s+/);
    isAnimatingTraversal = true;
    showToast(`▶ Animating ${type.toUpperCase()} traversal...`);

    for (const label of labels) {
      const nodeEl = document.querySelector(`.node-circle[data-label="${label}"]`);
      if (nodeEl) {
        nodeEl.classList.add('highlighted-node');
        await new Promise(r => setTimeout(r, 650));
        nodeEl.classList.remove('highlighted-node');
      }
    }
    isAnimatingTraversal = false;
    showToast(`✓ ${type.toUpperCase()} traversal animation complete.`);
  });
});

// -------------------------------------------------------------
// Interactive SVG Tree Visualizer
// -------------------------------------------------------------
showNullNodesToggle.addEventListener('change', () => {
  if (currentTreeData) {
    renderSvgTree(currentTreeData);
  }
});

resetZoomBtn.addEventListener('click', () => {
  if (currentTreeData) {
    renderSvgTree(currentTreeData);
  }
});

function renderSvgTree(rootNode) {
  treeSvg.innerHTML = '';

  if (!rootNode || rootNode.name === '#') {
    treeEmptyState.classList.remove('hidden');
    return;
  }
  treeEmptyState.classList.add('hidden');

  const showNulls = showNullNodesToggle.checked;
  const svgWidth = treeSvg.clientWidth || 900;
  const svgHeight = treeSvg.clientHeight || 420;

  // Calculate layout coordinates recursively
  const levels = [];
  function assignCoords(node, depth, leftBound, rightBound) {
    if (!node) return;
    if (!showNulls && node.name === '#') return;

    if (!levels[depth]) levels[depth] = [];
    const x = (leftBound + rightBound) / 2;
    const y = 50 + depth * 75;

    node.x = x;
    node.y = y;
    levels[depth].push(node);

    if (node.children && node.children.length > 0) {
      const leftChild = node.children.find(c => c.side === 'left');
      const rightChild = node.children.find(c => c.side === 'right');

      if (leftChild) {
        assignCoords(leftChild, depth + 1, leftBound, x);
      }
      if (rightChild) {
        assignCoords(rightChild, depth + 1, x, rightBound);
      }
    }
  }

  assignCoords(rootNode, 0, 40, svgWidth - 40);

  // Group for links and group for nodes
  const linksGroup = document.createElementNS('http://www.w3.org/2000/svg', 'g');
  const nodesGroup = document.createElementNS('http://www.w3.org/2000/svg', 'g');
  treeSvg.appendChild(linksGroup);
  treeSvg.appendChild(nodesGroup);

  // Draw links
  function drawLinks(node) {
    if (!node) return;
    if (!showNulls && node.name === '#') return;

    if (node.children) {
      for (const child of node.children) {
        if (!showNulls && child.name === '#') continue;
        if (child.x !== undefined && child.y !== undefined) {
          const path = document.createElementNS('http://www.w3.org/2000/svg', 'path');
          const isNull = child.name === '#';
          // Curved Bezier Branch
          const d = `M ${node.x} ${node.y} C ${node.x} ${(node.y + child.y) / 2}, ${child.x} ${(node.y + child.y) / 2}, ${child.x} ${child.y}`;
          path.setAttribute('d', d);
          path.setAttribute('class', `branch-link ${isNull ? 'null-link' : ''}`);
          linksGroup.appendChild(path);
          drawLinks(child);
        }
      }
    }
  }

  drawLinks(rootNode);

  // Draw nodes
  function drawNodes(node) {
    if (!node) return;
    if (!showNulls && node.name === '#') return;

    const g = document.createElementNS('http://www.w3.org/2000/svg', 'g');
    g.setAttribute('transform', `translate(${node.x}, ${node.y})`);

    const isNull = node.name === '#';
    const circle = document.createElementNS('http://www.w3.org/2000/svg', 'circle');
    circle.setAttribute('r', isNull ? '14' : '22');
    circle.setAttribute('class', `node-circle ${isNull ? 'null-node' : ''}`);
    circle.setAttribute('data-label', node.name);

    const text = document.createElementNS('http://www.w3.org/2000/svg', 'text');
    text.setAttribute('class', `node-text ${isNull ? 'null-text' : ''}`);
    text.textContent = node.name;

    // Tooltip title
    const title = document.createElementNS('http://www.w3.org/2000/svg', 'title');
    title.textContent = isNull ? 'Null Sentinel (Empty Branch)' : `Dynastic Monarch: ${node.name} (${node.side || 'root'})`;
    g.appendChild(title);

    g.appendChild(circle);
    g.appendChild(text);
    nodesGroup.appendChild(g);

    if (node.children) {
      for (const child of node.children) {
        drawNodes(child);
      }
    }
  }

  drawNodes(rootNode);
}

// -------------------------------------------------------------
// Auto-Run on Window Load
// -------------------------------------------------------------
window.addEventListener('DOMContentLoaded', () => {
  checkBackendHealth();
  updateTokenCounter();
  // Automatically render sample museum tree on start
  createTreeBtn.click();
});
