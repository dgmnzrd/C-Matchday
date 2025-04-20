document.addEventListener('DOMContentLoaded', () => {
    const form = document.getElementById('registerForm');
    form.addEventListener('submit', async e => {
        e.preventDefault();
        // seleccionamos los inputs por name
        const email = form.querySelector('input[name="email"]').value.trim();
        const password = form.querySelector('input[name="password"]').value.trim();

        console.log('Registrando:', { email, password });

        try {
            const res = await fetch('/register', {
                method: 'POST',
                headers: { 'Content-Type': 'application/json' },
                body: JSON.stringify({ email, password })
            });

            console.log('Status:', res.status, res.statusText);

            if (!res.ok) {
                const text = await res.text();
                throw new Error(`Fallo al registrar (${res.status}): ${text}`);
            }

            const data = await res.json();
            console.log('Respuesta JSON:', data);

            // guarda el token y redirige
            localStorage.setItem('idToken', data.idToken);
            window.location.href = 'matches.html';
        } catch (err) {
            console.error(err);
            alert(err.message);
        }
    });
});