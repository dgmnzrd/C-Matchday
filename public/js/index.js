// public/js/index.js

async function getJSON(url) {
    const res = await fetch(url);
    if (!res.ok) throw new Error(res.statusText);
    return res.json();
}

function formatDate(iso) {
    const d = new Date(iso);
    return (
        d.toLocaleDateString("es-ES", {
            day: "2-digit",
            month: "2-digit",
            year: "numeric",
        }) +
        " " +
        d.toLocaleTimeString("es-ES", {
            hour: "2-digit",
            minute: "2-digit",
        })
    );
}

// Pinta el <select> de ligas usando data.category
function renderCompetitions(data) {
    const sel = document.getElementById("competitionSelect");
    sel.innerHTML = '<option value="">Todas</option>';

    const cats = data.category;
    if (!Array.isArray(cats)) {
        console.error("Se esperaba data.category como arreglo", data);
        return;
    }

    cats.forEach((cat) => {
        const opt = document.createElement("option");
        opt.value = cat.id;
        opt.textContent = cat.name;
        sel.appendChild(opt);
    });
}

function renderMatches(matches) {
    const cont = document.getElementById("matches");
    cont.innerHTML = "";

    if (!Array.isArray(matches) || matches.length === 0) {
        cont.innerHTML =
            '<p class="text-red-500">No se encontraron partidos para esa liga/fecha.</p>';
        return;
    }

    matches.forEach((m, i) => {
        const card = document.createElement("div");
        card.className =
            "bg-white rounded-lg p-6 shadow-md hover:shadow-lg transition fade-in";
        card.style.animationDelay = `${i * 100}ms`;
        card.innerHTML = `
        <h3 class="text-xl font-semibold text-purple-800">
          ${m.homeTeam.name} vs ${m.awayTeam.name}
        </h3>
        <p class="text-3xl font-bold text-purple-600 mt-2">
          ${m.score.fullTime.homeTeam} – ${m.score.fullTime.awayTeam}
        </p>
        <p class="text-sm text-gray-500 mt-1">${formatDate(m.utcDate)}</p>
        <p class="text-xs uppercase text-gray-400 mt-2">${m.competition.name}</p>
      `;
        cont.appendChild(card);
    });
}

async function loadData() {
    try {
        // 1) Obtener ligas
        const data = await getJSON("/competitions");
        renderCompetitions(data);

        // 2) Partidos de hoy
        const today = new Date().toISOString().slice(0, 10);
        const matchesData = await getJSON(`/?dateFrom=${today}&dateTo=${today}`);
        renderMatches(matchesData.fixtures || []);
    } catch (err) {
        console.error(err);
        document.getElementById("matches").innerHTML = `
        <p class="text-red-500">Error cargando datos: ${err.message}</p>
      `;
    }
}

document.addEventListener("DOMContentLoaded", () => {
    // Inyecta footer
    fetch("/layouts/footer.html")
        .then((r) => r.text())
        .then((html) => {
            document.getElementById("footer-container").innerHTML = html;
        })
        .catch(console.error);

    // Inserta selector de ligas
    const hdr = document.querySelector("main");
    hdr.insertAdjacentHTML(
        "afterbegin",
        `
      <div class="mb-4">
        <label for="competitionSelect" class="font-medium">Liga:</label>
        <select id="competitionSelect" class="ml-2 p-2 border rounded">
          <option value="">Todas</option>
        </select>
      </div>
    `
    );

    // Al cambiar de liga, recarga partidos filtrados
    document
        .getElementById("competitionSelect")
        .addEventListener("change", async (e) => {
            const compId = e.target.value;
            try {
                const today = new Date().toISOString().slice(0, 10);
                let url = `/?dateFrom=${today}&dateTo=${today}`;
                if (compId) url += `&competitions=${compId}`;
                const data = await getJSON(url);
                renderMatches(data.fixtures || []);
            } catch (err) {
                console.error(err);
                document.getElementById("matches").innerHTML = `
            <p class="text-red-500">Error cargando partidos: ${err.message}</p>
          `;
            }
        });

    loadData();
});