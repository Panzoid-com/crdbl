import './App.css';
import { Sync } from './pages/Sync';
import { Tags } from './pages/Tags';
import { Snapshots } from './pages/Snapshots';
import { useEffect, useMemo, useState } from 'react';

const demos = [
  { name: "Sync", component: <Sync />, path: "#sync", default: true },
  { name: "Tags", component: <Tags />, path: "#tags" },
  { name: "Snapshots", component: <Snapshots />, path: "#snapshots" }
];

function App() {
  const [currentPath, setCurrentPath] = useState(window.location.hash);
  const currentDemo = useMemo(() => demos.find(demo => currentPath === demo.path) ||
    demos.find(demo => demo.default), [currentPath]);

  useEffect(() => {
    const handleHashChange = () => {
      setCurrentPath(window.location.hash);
    };
    window.addEventListener("hashchange", handleHashChange);

    return () => {
      window.removeEventListener("hashchange", handleHashChange);
    };
  }, []);

  return <div className="App">
    <div className="navbar">
      {demos.map((demo, i) =>
        <a key={i} href={demo.path} className={currentDemo === demo ? "current" : ""}>
          {demo.name}
        </a>)}
    </div>
    <div>
      {demos.find(demo => currentDemo === demo)?.component}
    </div>
  </div>;
}

export default App;
