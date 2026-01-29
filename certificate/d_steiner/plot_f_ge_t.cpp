# include <bits/stdc++.h>
using namespace std;
using ld = double;
using ull = unsigned long long;
const ld rho = 0.8559, INF = 1e18, eps = 1e-6;
const int F_VAL = 2; // 2 for f >= t
const int n = 7;

template<typename... Args>
ld max(ld a, ld b, ld c, Args... args)
{
	return max({a, b, c, args...});
}

constexpr bool isinfinity(ld x)
{
	// return isinf(x);
	return bit_cast<ull>(x) == bit_cast<ull>(numeric_limits<ld>::infinity());
}

using Box = array<array<ld, 2>, n>;
using Point = array<ld, n>;

const array<string, n> vars = {"b", "c", "d", "s", "t", "u", "e"};

bool glob_cond(ld b, ld c, ld d, ld s, ld t, ld u, ld e)
{
	return max(c,d) <= max(b,(ld)1);
}

template<int>
struct F;

# include "F0"
# include "F1"
# include "F2"
# include "F3"
# include "F4"
# include "F5"
# include "F6"
# include "F7"
# include "F8"

const int m = M8;

template<template<int> class F, int N>
auto eval_all(ull mono_mask, ld b, ld c, ld d, ld s, ld t, ld u, ld e)
{
	ld f = t;
	auto imp = [&]<int... I>(integer_sequence<int, I...>) -> array<ld, sizeof...(I)> {
		return {F<I>{}(mono_mask, b, c, d, s, t, u, e, f)...};
	};
	return imp(make_integer_sequence<int, N>{});
}

template<typename T>
struct TaskPool
{
	queue<T> que;
	size_t working;
	bool stop;
	mutex mut;
	condition_variable cv;

	TaskPool() : working(0), stop(false) {}
	optional<T> try_pop()
	{
		unique_lock lock(mut);
		cv.wait(lock, [&]{return stop || !que.empty();});
		if(stop) return {};
		auto head = que.front(); que.pop();
		working++;
		return head;
	}
	template<typename Arg>
	void push(Arg &&arg)
	{
		lock_guard lock(mut);
		que.push(forward<Arg>(arg));
		cv.notify_one();
	}
	template<typename... Args>
	void push_children(Args&&... args)
	{
		lock_guard lock(mut);
		(que.push(forward<Args>(args)), ...);
		working--;
		if(que.empty() && working == 0) stop = true;
		cv.notify_all();
	}
};

int main()
{
	ull iter = 0;
	TaskPool<Box> pool;
	for(ull mask = 0; mask < 1ull<<n; mask++)
	{
		Box box;
		for(int i = 0; i < n; i++)
			if((mask >> i) & 1) box[i][0] = 1, box[i][1] = numeric_limits<ld>::infinity();
			else box[i][0] = 0, box[i][1] = 1;
		pool.push(move(box));
	}

	vector<pair<Box, int>> certified;

	auto worker = [&]()
	{
		vector<Point> corners;
		corners.reserve(1ull<<n);
		array<ld, m> maxn;
		unique_lock lock(pool.mut, defer_lock);
		while(true)
		{
			lock.lock();
			iter++;
			if(iter % 10000 == 0) cerr<<iter<<" "<<certified.size()<<endl;
			if(iter > 1e9) break;
			lock.unlock();
			auto h = pool.try_pop();
			if(!h.has_value()) break;
			auto box = *h;
			bool not_in_domain = true;
			corners.clear();
			for(ull mask = 0; mask < 1ull<<n; mask++)
			{
				Point p;
				bool no_inf = true;
				for(int i = 0; i < n; i++)
				{
					p[i] = (mask >> i) & 1 ? box[i][0] : box[i][1];
					if(isinfinity(p[i])) no_inf = false;
				}
				if(apply(glob_cond, p)) not_in_domain = false;
				if(no_inf) corners.push_back(move(p));
			}
			if(not_in_domain)
			{
				lock.lock();
				certified.emplace_back(move(box), -1);
				lock.unlock();
				pool.push_children();
				continue;
			}
			ull mono_mask = 0;
			for(int i = 0; i < n; i++)
				if(isinfinity(box[i][1])) mono_mask |= 1ull<<i;
			maxn.fill(-INF);
			auto eval = [mono_mask]<typename... Args>(Args&&... args) {
				return eval_all<F, m>(mono_mask, forward<Args>(args)...);
			};
			for(auto &&p : corners)
			{
				auto val = apply(eval, p);
				for(int i = 0; i < m; i++) maxn[i] = max(maxn[i], val[i]);
			}
			auto id = min_element(maxn.begin(), maxn.end()) - maxn.begin();
			if(maxn[id] < -eps)
			{
				lock.lock();
				certified.emplace_back(move(box), id);
				lock.unlock();
				pool.push_children();
				continue;
			}
			auto dim = max_element(box.begin(), box.end(), [](auto &&x, auto &&y) {
				ld sepx = isinfinity(x[1]) ? 1 / x[0] : x[1] - x[0];
				ld sepy = isinfinity(y[1]) ? 1 / y[0] : y[1] - y[0];
				return sepx < sepy;
			}) - box.begin();
			Box boxl = box, boxr = box;
			if(isinfinity(box[dim][1])) boxl[dim][1] = boxr[dim][0] = 2 * box[dim][0];
			else boxl[dim][1] = boxr[dim][0] = (box[dim][0] + box[dim][1]) / 2;
			pool.push_children(move(boxl), move(boxr));
		}
	};

	auto n_threads = min(thread::hardware_concurrency(), 50u);
	cout<<"n_threads = "<<n_threads<<endl;
	vector<thread> workers;
	workers.reserve(n_threads);
	for(unsigned _ = 0; _ < n_threads; _++) workers.emplace_back(worker);
	for(auto &&t : workers) t.join();

	cout<<fixed<<setprecision(20);
	Point maxn, minn;
	maxn.fill(0); minn.fill(INF);
	cout<<certified.size()<<" "<<pool.que.size()<<endl;
	ld unproven_area = 0, box_area = 1;
	int ccc = 0;
	while(!pool.que.empty())
	{
		auto box = pool.que.front(); pool.que.pop();
		ld vol = 1;
		if((++ccc) <= 10)
		{
			for(int i = 0; i < n; i++) cout<<box[i][0]<<" "<<box[i][1]<<endl;
			cout<<endl;
		}
		for(int i = 0; i < n; i++)
		{
			vol *= box[i][1] - box[i][0];
			minn[i] = min(minn[i], box[i][0]);
			maxn[i] = max(maxn[i], box[i][1]);
		}
		unproven_area += vol;
	}
	for(int i = 0; i < n; i++) box_area *= max(maxn[i] - minn[i], 0.);
	cout<<unproven_area<<" "<<box_area<<endl;
	for(int i = 0; i < n; i++) cout<<vars[i]<<" in ["<<minn[i]<<", "<<maxn[i]<<"]"<<endl;

	// cout<<file_name<<endl;
	// ofstream fout(file_name);
	// fout<<fixed<<setprecision(20);
	// for(int i = 0; i < n; i++) fout<<vars[i]<<"_min"<<","<<vars[i]<<"_max"<<",";
	// fout<<"id\n";
	// for(auto &&[box, id] : certified)
	// {
	// 	for(int i = 0; i < n; i++) fout<<box[i][0]<<","<<box[i][1]<<",";
	// 	fout<<id<<"\n";
	// }
	return 0;
}
