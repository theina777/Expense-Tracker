// Minimalist Expense Tracker - WebAssembly Frontend

const App = {
    api: null,
    chartInstance: null,
    
    init() {
        console.log("Initializing Expense Tracker...");
        // Emscripten exposes the 'Module' object when compiling. 
        // We override its initialization callback to know when C is ready.
        if (typeof Module === 'undefined') {
            console.warn("Waiting for WebAssembly script to load...");
            setTimeout(() => this.init(), 100);
            return;
        }

        Module.onRuntimeInitialized = () => {
            console.log("WebAssembly Core Ready!");
            this.bindAPI();
            this.setupEvents();
            this.updateDashboard();
        };
    },

    bindAPI() {
        // Map the C functions to Javascript using cwrap
        try {
            this.api = {
                addExpense: Module.cwrap('api_addExpense', 'number', ['string', 'string', 'number', 'string']),
                getExpenseCount: Module.cwrap('api_getExpenseCount', 'number', []),
                getExpenseDate: Module.cwrap('api_getExpenseDate', 'string', ['number']),
                getExpenseCategory: Module.cwrap('api_getExpenseCategory', 'string', ['number']),
                getExpenseAmount: Module.cwrap('api_getExpenseAmount', 'number', ['number']),
                getExpenseNote: Module.cwrap('api_getExpenseNote', 'string', ['number']),
                getBudget: Module.cwrap('api_getBudget', 'number', []),
                setBudget: Module.cwrap('api_setBudget', null, ['number']),
                deleteExpense: Module.cwrap('api_deleteExpense', 'number', ['number'])
            };
        } catch(e) {
            console.error("Error binding C API. Make sure you used the correct compilation flags.", e);
        }
    },

    setupEvents() {
        // Form Submission
        document.getElementById('add-expense-form').addEventListener('submit', (e) => {
            e.preventDefault();
            
            const date = document.getElementById('date-input').value;
            const category = document.getElementById('category-input').value;
            const amount = parseFloat(document.getElementById('amount-input').value);
            const note = document.getElementById('note-input').value || "";

            // Call WebAssembly API!
            const success = this.api.addExpense(date, category, amount, note);
            
            if (success) {
                document.getElementById('add-expense-form').reset();
                this.updateDashboard();
            } else {
                alert("Failed to add expense. Reached maximum limit.");
            }
        });

        // Budget Modal Logic
        const modal = document.getElementById('budget-modal');
        document.getElementById('set-budget-btn').addEventListener('click', () => {
            document.getElementById('new-budget-input').value = this.api.getBudget().toFixed(2);
            modal.classList.add('active');
        });
        
        document.getElementById('cancel-budget-btn').addEventListener('click', () => {
            modal.classList.remove('active');
        });

        document.getElementById('save-budget-btn').addEventListener('click', () => {
            const newBudget = parseFloat(document.getElementById('new-budget-input').value);
            if (!isNaN(newBudget) && newBudget >= 0) {
                this.api.setBudget(newBudget);
                modal.classList.remove('active');
                this.updateDashboard();
            }
        });

        // Interactive Background Mesh Gradient Logic
        document.addEventListener('mousemove', (e) => {
            const x = (e.clientX / window.innerWidth) - 0.5;
            const y = (e.clientY / window.innerHeight) - 0.5;
            
            // Use CSS variables to avoid conflicting with the CSS @keyframes transform animation!
            document.documentElement.style.setProperty('--mouse-x', x);
            document.documentElement.style.setProperty('--mouse-y', y);
            
            // Exact pixels for the cursor spotlight gradient
            document.documentElement.style.setProperty('--mouse-px-x', e.clientX + 'px');
            document.documentElement.style.setProperty('--mouse-px-y', e.clientY + 'px');
        });
    },

    formatCurrency(amount) {
        return '₹' + parseFloat(amount).toFixed(2);
    },

    updateDashboard() {
        const count = this.api.getExpenseCount();
        const budget = this.api.getBudget();
        
        let totalExpenses = 0;
        const categoryTotals = {};
        const tbody = document.getElementById('expenses-tbody');
        tbody.innerHTML = '';

        // Iterate backward to show newest first
        for (let i = count - 1; i >= 0; i--) {
            const date = this.api.getExpenseDate(i);
            const category = this.api.getExpenseCategory(i);
            const amount = this.api.getExpenseAmount(i);
            const note = this.api.getExpenseNote(i);
            
            totalExpenses += amount;
            
            // Group for chart
            const catKey = category.trim().charAt(0).toUpperCase() + category.trim().slice(1).toLowerCase();
            categoryTotals[catKey] = (categoryTotals[catKey] || 0) + amount;

            const tr = document.createElement('tr');
            tr.innerHTML = `
                <td>${date}</td>
                <td><span class="badge">${category}</span></td>
                <td style="color: var(--text-muted)">${note}</td>
                <td class="amount-col">${this.formatCurrency(amount)}</td>
                <td style="text-align: right">
                    <button class="del-btn" onclick="App.deleteExpense(${i})">×</button>
                </td>
            `;
            tbody.appendChild(tr);
        }

        // Update UI empty state
        document.getElementById('empty-state').style.display = count === 0 ? 'block' : 'none';
        document.getElementById('expenses-table').parentElement.style.display = count === 0 ? 'none' : 'block';
        document.getElementById('expense-count-badge').textContent = count;

        // Update Dashboard Cards
        document.getElementById('budget-display').textContent = this.formatCurrency(budget);
        document.getElementById('total-display').textContent = this.formatCurrency(totalExpenses);
        
        const remaining = budget - totalExpenses;
        const remainingDisplay = document.getElementById('remaining-display');
        
        if (budget > 0) {
            remainingDisplay.textContent = this.formatCurrency(remaining);
            if (remaining < 0) {
                remainingDisplay.classList.add('status-danger');
                remainingDisplay.classList.remove('status-success');
            } else {
                remainingDisplay.classList.add('status-success');
                remainingDisplay.classList.remove('status-danger');
            }
        } else {
            remainingDisplay.textContent = "Not Set";
            remainingDisplay.className = "";
        }
        
        this.updateChart(categoryTotals);
        this.updateInsights(totalExpenses, budget, categoryTotals, count);
    },

    updateInsights(total, budget, categories, count) {
        const insightBox = document.getElementById('insight-alert');
        insightBox.className = 'insight-alert'; // reset classes
        
        if (count === 0) {
            insightBox.classList.add('insight-info');
            insightBox.innerHTML = '💡 <strong>Tip:</strong> You are starting fresh! Add your first expense below to see your data come to life.';
            return;
        }

        if (budget === 0) {
            insightBox.classList.add('insight-info');
            insightBox.innerHTML = `💡 <strong>Tip:</strong> You haven't set a monthly budget yet! Setting a budget helps you automatically track your savings goals.`;
            return;
        }

        // Find highest spending category
        let highestCat = '';
        let highestAmt = 0;
        for (const [cat, amt] of Object.entries(categories)) {
            if (amt > highestAmt) {
                highestAmt = amt;
                highestCat = cat;
            }
        }

        const percentage = (total / budget) * 100;

        if (total > budget) {
            insightBox.classList.add('insight-danger');
            insightBox.innerHTML = `⚠️ <strong>Over Budget!</strong> You have exceeded your budget by ${this.formatCurrency(total - budget)}. Your biggest drain is currently <strong>${highestCat}</strong> (${this.formatCurrency(highestAmt)}).`;
        } else if (percentage >= 80) {
            insightBox.classList.add('insight-warning');
            insightBox.innerHTML = `⚡ <strong>Careful!</strong> You have spent ${percentage.toFixed(0)}% of your budget. Try cutting back on <strong>${highestCat}</strong> to stay safe this month.`;
        } else {
            insightBox.classList.add('insight-success');
            insightBox.innerHTML = `🎉 <strong>Great job!</strong> You are well within your budget! Your highest spending is on <strong>${highestCat}</strong>, but you still have ${this.formatCurrency(budget - total)} left to enjoy or save.`;
        }
    },

    updateChart(categoryTotals) {
        const labels = Object.keys(categoryTotals);
        const data = Object.values(categoryTotals);

        if (labels.length === 0) {
            document.getElementById('expense-chart').style.display = 'none';
            document.getElementById('no-chart-data').style.display = 'block';
            return;
        }

        document.getElementById('expense-chart').style.display = 'block';
        document.getElementById('no-chart-data').style.display = 'none';

        if (this.chartInstance) {
            this.chartInstance.data.labels = labels;
            this.chartInstance.data.datasets[0].data = data;
            this.chartInstance.update();
        } else {
            const ctx = document.getElementById('expense-chart').getContext('2d');
            this.chartInstance = new Chart(ctx, {
                type: 'doughnut',
                data: {
                    labels: labels,
                    datasets: [{
                        data: data,
                        backgroundColor: [
                            '#0f172a', '#3b82f6', '#10b981', '#f59e0b', '#ef4444', 
                            '#8b5cf6', '#ec4899', '#14b8a6', '#f97316', '#64748b'
                        ],
                        borderWidth: 0,
                        hoverOffset: 4
                    }]
                },
                options: {
                    responsive: true,
                    maintainAspectRatio: false,
                    plugins: {
                        legend: { position: 'bottom', labels: { boxWidth: 12, padding: 15, font: { family: 'Inter' } } }
                    },
                    cutout: '75%'
                }
            });
        }
    },

    deleteExpense(index) {
        if (confirm("Are you sure you want to delete this expense?")) {
            this.api.deleteExpense(index);
            this.updateDashboard();
        }
    }
};

App.init();
